#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/details/os.h>
#include <dtCore/dtThread>
#include "dtCore/src/dtLog/dtRtLog.hpp"

namespace dt {

namespace Log {
// ─── RtLog ──────────────────────────────────────────────────────────────────

struct RtLog::ThreadInfo_Impl {
    Thread::ThreadInfo threadInfo;
};

RtLog::RtLog() noexcept
    : m_logger(nullptr),
      m_tui(nullptr),
      m_tuiLastRender_ns(0),
      m_lastFlush_ns(0),
      m_timebase{},
      m_initialized(false),
      m_contBufLen(0),
      m_contLevel(spdlog::level::info)
{
    m_level.store(static_cast<int>(LogLevel::trace), std::memory_order_relaxed);
    m_dropCount.store(0, std::memory_order_relaxed);
    m_syncSeq.store(0, std::memory_order_relaxed);
    m_syncAck.store(0, std::memory_order_relaxed);
    m_logThreadInfo = std::make_unique<ThreadInfo_Impl>();
}

RtLog::~RtLog() = default;

void RtLog::Initialize(
    const std::string &logName,
    const std::string &fileBasename,
    bool enableTui,
    int threadCpuId,
    size_t maxFiles,
    size_t maxFileSize,
    int threadPriority,
    size_t threadStack,
    bool annotDatetime,
    bool truncate)
{
    auto &m_instance = Instance();
    if (m_instance.m_initialized.load(std::memory_order_acquire))   // check initialize state
    {
        m_instance.LogRt(LogLevel::err, "[RtLog] Initialize() is already called!!!");
        return;
    }

    // create spdlog logger with appropriate sinks
    m_instance.m_logger = std::make_shared<spdlog::logger>(logName);
    m_instance.m_logger->sinks().clear();

    // create tui instance if enabled
    if (enableTui)
    {
        m_instance.m_tui = std::make_shared<RtTui>();
        if (m_instance.m_tui->Init())
        {
            auto tui_sink = std::make_shared<TuiSink>(m_instance.m_tui);
            tui_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
            m_instance.m_logger->sinks().push_back(tui_sink);
        }
        else
        {
            // TUI init failed: fall back to colored stdout
            m_instance.m_tui.reset();
            auto console_sink = std::make_shared<ColorStdoutSinkMt>();
            console_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
            m_instance.m_logger->sinks().push_back(console_sink);
            LogRaw(LogLevel::err, "TUI initialize failed -> use default stdout");
        }
    }
    // create default color stdout sink
    else
    {
        auto console_sink = std::make_shared<ColorStdoutSinkMt>();
        console_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
        m_instance.m_logger->sinks().push_back(console_sink);
    }

    // basic file sink
    if (!fileBasename.empty() && (fileBasename != "_STDOUT_"))
    {
        spdlog::filename_t filename = fileBasename;
        if (annotDatetime)
        {
            filename = m_instance.AnnotateFilenameDatetime(fileBasename);
            auto [dname, fname] = m_instance.SplitByDirectory(filename);
            // check folder
            std::error_code ec;
            auto result = m_instance.EnsureDirectoryExistes(dname, ec);
            if (!result)
            {
                m_instance.m_logger->log(spdlog::level::err, "Cannot create directory '{}': {}", dname, ec.message());
            }

            (void)remove(fileBasename.c_str());
            auto rtn = symlink(fname.c_str(), fileBasename.c_str());
            if (rtn < 0)
            {
                // Cannot create symlink to this log file. Log a warning to the console sink if available.
                if (m_instance.m_logger)
                {
                    m_instance.m_logger->log(spdlog::level::warn,
                        "Cannot create symlink '{}' → '{}': {}", fileBasename, fname, strerror(errno));
                }
            }
        }

        auto file_sink = std::make_shared<BasicFileSinkMt>(filename, maxFileSize, maxFiles, truncate);
        file_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
        m_instance.m_logger->sinks().push_back(file_sink);
    }

    spdlog::set_default_logger(m_instance.m_logger);
    m_instance.m_timebase   = TimeBase::Capture();
    m_instance.m_patternStr = "%^[%L][%H:%M:%S.%f]%$ %v";
    m_instance.m_contBufLen = 0;

    // Create log thread
    m_instance.m_logThreadInfo->threadInfo.name = "RtLogThread";
    m_instance.m_logThreadInfo->threadInfo.stackSz = threadStack;
    m_instance.m_logThreadInfo->threadInfo.cpuIdx = threadCpuId;
    m_instance.m_logThreadInfo->threadInfo.priority = threadPriority;
    m_instance.m_logThreadInfo->threadInfo.procFunc = PollLogQueue;
    m_instance.m_logThreadInfo->threadInfo.procFuncArg = nullptr;

    m_instance.m_logThreadRun.store(true, std::memory_order_release);
    int result = Thread::CreateThread(m_instance.m_logThreadInfo->threadInfo, 
                                    (m_instance.m_logThreadInfo->threadInfo.priority > 0) ? true : false, 
                                    false);
    if (result == 0)
    {
        m_instance.m_initialized.store(true, std::memory_order_release);
    }
    else
    {
        m_instance.m_logThreadRun.store(false, std::memory_order_release);
        m_instance.m_logger->log(spdlog::level::err, "Cannot create log thread: {}", strerror(errno));
    }
}

void RtLog::Create(
    const std::string &logName,
    const std::string &fileBasename,
    bool annotDatetime,
    bool truncate,
    size_t maxFiles,
    size_t maxFileSize)
{
    auto &inst = Instance();
    auto logger = std::make_shared<spdlog::logger>(logName);
    logger->sinks().clear();

    if (fileBasename == "_STDOUT_")
    {
        auto console_sink = std::make_shared<ColorStdoutSinkMt>();
        console_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
        logger->sinks().push_back(console_sink);
    }
    else
    {
        spdlog::filename_t filename = fileBasename;
        if (annotDatetime)
        {
            filename = inst.AnnotateFilenameDatetime(fileBasename);
            auto [dname, fname] = inst.SplitByDirectory(filename);
            // check folder
            std::error_code ec;
            auto result = inst.EnsureDirectoryExistes(dname, ec);
            if (!result)
            {
                inst.m_logger->log(spdlog::level::err, "Cannot create directory '{}': {}", dname, ec.message());
            }

            (void)remove(fileBasename.c_str());
            auto rtn = symlink(fname.c_str(), fileBasename.c_str());
            if (rtn < 0)
            {
                if (inst.m_logger)
                {
                    inst.m_logger->log(spdlog::level::warn,
                        "Cannot create symlink '{}' -> '{}': {}", fileBasename, fname, strerror(errno));
                }
            }
        }
        auto file_sink = std::make_shared<BasicFileSinkMt>(filename, maxFileSize, maxFiles, truncate);
        file_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
        logger->sinks().push_back(file_sink);
    }

    spdlog::register_logger(logger);
}

void RtLog::Terminate()
{
    auto &m_instance = Instance();

    if (m_instance.m_initialized.load(std::memory_order_acquire) == false)
    {
        return;
    }

    // Report any messages that were dropped during operation
    uint64_t drops = m_instance.DropCount();
    if (drops > 0)
    {
        m_instance.m_logger->log(spdlog::level::warn, "CloseLogger: RT log queue dropped %llu messages during operation", (unsigned long long)drops);
    }

    // delete thread
    m_instance.m_logThreadRun.store(false, std::memory_order_release);
    int result = Thread::DeleteThread(m_instance.m_logThreadInfo->threadInfo);

    // last flush
    if (result == 0)
    {
        m_instance.m_logThreadInfo->threadInfo.id = {};
        m_instance.DrainAll();
        m_instance.FlushContLines(true);  // flush any pending LOG_CONT output
    }

    // Stop TUI if enabled
    if (m_instance.m_tui)
    {
        m_instance.m_tui->Stop();
        m_instance.m_tui.reset();
    }

    m_instance.m_initialized.store(false, std::memory_order_release);

    // flush all pending log messages
    spdlog::shutdown();
}

void RtLog::FlushOn(LogLevel lvl)
{
    Instance().LogRt(LogLevel::debug, "[RtLog] No need: FlushOn()");
}

void RtLog::FlushOn(const std::string &logger_name, LogLevel lvl)
{
    Instance().LogRt(LogLevel::debug, "[RtLog:%s] No need: FlushOn()", logger_name.c_str());
}

void RtLog::SetLogLevel(LogLevel lvl)
{
    Instance().SetLevel(lvl);
}

void RtLog::SetLogLevel(const std::string &logger_name, LogLevel lvl)
{
    std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
    if (logger) 
    {
        logger->set_level(lvl);
    }
}

void RtLog::SetLogPattern(LogPattern pattern, const std::string &delimiter)
{
    std::string pattern_str = "%^";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimiter :
                   (pattern & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimiter : "";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimiter :
                   (pattern & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimiter :
                   (pattern & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimiter :
                   (pattern & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimiter : "";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimiter : "";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimiter : "";
    pattern_str += "%$%v";
    Sync();  // flush previous queued messages using old pattern
    auto &inst = Instance();
    inst.m_patternStr = pattern_str;
    if (inst.m_logger) 
    {
        inst.m_logger->set_pattern(pattern_str);
    }
}

void RtLog::SetLogPattern(const std::string &logger_name, LogPattern pattern, const std::string &delimiter)
{
    std::string pattern_str = "%^";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimiter :
                    (pattern & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimiter : "";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimiter :
                    (pattern & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimiter :
                    (pattern & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimiter :
                    (pattern & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimiter : "";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimiter : "";
    pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimiter : "";
    pattern_str += "%$%v";
    Sync();  // drain queued messages and flush all sinks before changing the pattern
    std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
    if (logger)
    {
        // Flush the named logger's sink buffer so that already-formatted (old-pattern)
        // bytes are written out before the formatter is replaced.  Without this, the
        // sink buffer would be flushed later with new-pattern messages appended,
        // mixing two formats in the output.
        logger->flush();
        logger->set_pattern(pattern_str);
    }
}

void RtLog::SetLogPattern(const std::string &raw_pattern)
{
    Sync();  // flush previous queued messages using old pattern
    auto &inst = Instance();
    inst.m_patternStr = raw_pattern;
    if (inst.m_logger) 
    {
        inst.m_logger->set_pattern(raw_pattern);
    }
}

void RtLog::SetLogPattern(const std::string &logger_name, const std::string &raw_pattern)
{
    Sync();  // drain queued messages and flush all sinks before changing the pattern
    std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
    if (logger)
    {
        logger->flush();
        logger->set_pattern(raw_pattern);
    }
}

std::shared_ptr<Log::RtTui> RtLog::GetTui() const noexcept
{
    return m_tui;
}

char RtLog::GetPendingKey() const noexcept
{
    return m_tui ? m_tui->PopPendingKey() : 0;
}

void RtLog::TuiSetGroupNoHeader(int layoutIdx, int groupIdx) noexcept
{
    if (m_tui)
    {
        m_tui->SetGroupNoHeader(layoutIdx, groupIdx);
    }
}

void RtLog::TuiSetTextRow(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *text) noexcept
{
    if (m_tui)
    {
        m_tui->SetTextRow(layoutIdx, groupIdx, rowIdx, label, text);
    }
}

void RtLog::TuiSetTextRowFmt(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *fmt, ...) noexcept
{
    if (!m_tui)
    {
        return;
    }

    char buf[Log::RtTui::TUI_TEXT_ROW_LEN];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    m_tui->SetTextRow(layoutIdx, groupIdx, rowIdx, label, buf);
}

void RtLog::TuiSetLayoutName(int layoutIdx, const char *name) noexcept
{
    if (m_tui)
    {
        m_tui->SetLayoutName(layoutIdx, name);
    }
}

bool RtLog::IsInitialized() const noexcept
{
    return m_initialized.load(std::memory_order_acquire);
}

void *RtLog::PollLogQueue(void *pArg) noexcept
{
    auto &m_instance = Instance();

    while (m_instance.m_logThreadRun.load(std::memory_order_acquire))
    {
        Instance().Poll();
    }

    return nullptr;
}

void RtLog::RefreshTimebase() noexcept
{
    m_timebase = TimeBase::Capture();
}

size_t RtLog::DrainAll() noexcept
{
    size_t count = 0;
    Entry entry;
    while (m_queue.TryPop(entry))
    {
        FlushEntry(entry);
        ++count;
    }

    return count;
}

void RtLog::Sync() noexcept
{
    auto &inst = Instance();
    if (!inst.m_initialized.load(std::memory_order_acquire))
    {
        return;
    }

    uint64_t target = inst.m_syncSeq.fetch_add(1, std::memory_order_acq_rel) + 1;

    while (inst.m_syncAck.load(std::memory_order_acquire) < target)
    {
        if (inst.m_logThreadRun.load(std::memory_order_acquire) == false)
        {
            break;
        }
        
        struct timespec ts{0, 500000L};  // 500 μs
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
    }

    // sink 내부 버퍼 flush (ColorStdoutSinkT는 64KB 내부 버퍼를 사용)
    if (inst.m_logger) inst.m_logger->flush();
}

void RtLog::LogRaw(LogLevel lvl, const char *fmt, ...) noexcept
{
    char buf[512];

    // Timestamp (Cobalt-wrapped, no mode switch)
    struct timespec ts{};
    clock_gettime(CLOCK_REALTIME, &ts);
    auto sod = ts.tv_sec % 86400L;
    int  hh  = (int)(sod / 3600);
    int  mm  = (int)((sod % 3600) / 60);
    int  ss  = (int)(sod % 60);
    int  us  = (int)(ts.tv_nsec / 1000);

    char lc;
    switch (lvl) {
        case spdlog::level::trace:    lc = 'T'; break;
        case spdlog::level::debug:    lc = 'D'; break;
        case spdlog::level::info:     lc = 'I'; break;
        case spdlog::level::warn:     lc = 'W'; break;
        case spdlog::level::err:      lc = 'E'; break;
        case spdlog::level::critical: lc = 'C'; break;
        default:                      lc = '?'; break;
    }

    // Header: color-coded level + timestamp + bold [RAW] tag
    // SinkColorFor() is defined in the same dt namespace, no allocation
    const char* color = SinkColorFor(lvl);
    int hdr = snprintf(buf, sizeof(buf),
        "%s[%c][%02d:%02d:%02d.%06d]\033[0m\033[1m[RAW]\033[0m ",
        color, lc, hh, mm, ss, us);
    if (hdr < 0 || hdr >= (int)sizeof(buf))
    {
        hdr = 0;
    }

    if (!fmt)
    {
        fmt = "(null)";
    }

    // Message
    va_list args;
    va_start(args, fmt);
    int msg_n = vsnprintf(buf + hdr, sizeof(buf) - (size_t)hdr - 1, fmt, args);
    va_end(args);
    if (msg_n < 0)
    {
        msg_n = 0;
    }

    size_t total = (size_t)hdr + (size_t)msg_n;
    if (total >= sizeof(buf) - 1)
    {
        total = sizeof(buf) - 2;
    }
    buf[total++] = '\n';

    // Atomic write: messages ≤ PIPE_BUF (4096 B) are never interleaved on Linux
    ssize_t n = ::write(STDERR_FILENO, buf, total);
    if (n < 0) {}   // NOP: write error is occurred but it don't needed to be reported
}

void RtLog::LogRtV(LogLevel lvl, const char* format, va_list args) noexcept
{
    if (!m_initialized.load(std::memory_order_acquire))
    {
        return;
    }

    if (static_cast<int>(lvl) < m_level.load(std::memory_order_relaxed))
    {
        return;
    }

    Entry entry;
    entry.SetV(lvl, MonoNow_ns(), format, args);
    Enqueue(entry);
}

void RtLog::LogRtNamedV(const char *loggerName, LogLevel lvl, const char *format, va_list args) noexcept
{
    if (!m_initialized.load(std::memory_order_acquire))
    {
        return;
    }

    if (static_cast<int>(lvl) < m_level.load(std::memory_order_relaxed))
    {
        return;
    }

    Entry entry;
    entry.SetV(lvl, MonoNow_ns(), format, args);
    strncpy(entry.loggerName, loggerName, sizeof(entry.loggerName) - 1);
    entry.loggerName[sizeof(entry.loggerName) - 1] = '\0';
    Enqueue(entry);
}

void RtLog::SetLevel(LogLevel lvl) noexcept
{
    m_level.store(static_cast<int>(lvl), std::memory_order_relaxed);
    // Synchronize with spdlog logger level
    if (m_logger)
    {
        m_logger->set_level(lvl);
    }
}

LogLevel RtLog::GetLevel() const noexcept
{
    return static_cast<LogLevel>(m_level.load(std::memory_order_relaxed));
}

uint64_t RtLog::DropCount() const noexcept
{
    return m_dropCount.load(std::memory_order_relaxed);
}

// Get approximate number of messages currently in queue
size_t RtLog::QueueSize() const noexcept
{
    return m_queue.ApproxSize();
}

// Get queue utilization percentage (0-100)
size_t RtLog::QueueUtilization() const noexcept
{
    size_t size = m_queue.ApproxSize();
    return ((size * 100) / QueueType::Capacity());
}

RtLog::QueueStats RtLog::GetQueueStats() const noexcept
{
    size_t size = m_queue.ApproxSize();
    return QueueStats{
        .current_size = size,
        .capacity = QueueType::Capacity(),
        .utilization_pct = (size * 100) / QueueType::Capacity(),
        .total_drops = m_dropCount.load(std::memory_order_relaxed)
    };
}

void RtLog::Poll() noexcept
{
    // Sleep in the non-RT log thread, then drain all queued entries.
    // The RT producer path is syscall-free: it only writes to the lock-free queue.

    // Check queue size BEFORE draining to determine next polling interval
    size_t queueSizeBefore = m_queue.ApproxSize();

    // Drain all queued entries (TuiSinkT pushes to TUI queue here)
    size_t count = DrainAll();

    // Acknowledge any pending Sync() requests.
    // Sync()는 이 store를 감지할 때까지 대기하며, 이 시점에 DrainAll()이 완료된 것이 보장됨.
    {
        uint64_t pending = m_syncSeq.load(std::memory_order_acquire);
        if (pending > m_syncAck.load(std::memory_order_relaxed))
        {
            m_syncAck.store(pending, std::memory_order_release);
        }
    }

    // Rate-limit flush() to 100 ms regardless of message count.
    // Flushing even when count == 0 drains EAGAIN-retained bytes left in the sink buffer,
    // preventing the last few messages before a quiet period from being stuck.
    const int64_t now_ns = MonoNow_ns();
    if (m_logger && now_ns - m_lastFlush_ns >= 100'000'000LL)
    {  // 100 ms
        m_logger->flush();
        m_lastFlush_ns = now_ns;
    }

    // TUI tick: 25 Hz (40 ms) rate-limiter — drains queue, handles keys, renders
    if (m_tui)
    {
        if (now_ns - m_tuiLastRender_ns >= 40'000'000LL)
        {
            m_tui->Tick();
            m_tuiLastRender_ns = now_ns;
        }
    }

    // Check queue size AFTER draining to see if we're keeping up
    size_t queueSizeAfter = m_queue.ApproxSize();

    // Adaptive polling: adjust interval based on queue pressure
    long interval_ns;

    if (queueSizeAfter > QueueType::Capacity() / 2)
    {
        interval_ns = 100'000L;   // 100 μs: queue >50% — drain as fast as possible
    }
    else if (count > 0 && queueSizeAfter >= queueSizeBefore)
    {
        interval_ns = 100'000L;  // 100 μs: queue not shrinking after drain (falling behind)
    }
    else if (count > 0)
    {
        interval_ns = 500'000L;  // 500 μs: draining normally
    }
    else
    {
        interval_ns = RtLogConstant::POLL_INTERVAL_NS;  // 1 ms: idle (avoids 0>=0 false-positive)
    }

    struct timespec ts{0, interval_ns};
    clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
}

void RtLog::FlushEntry(const Entry &entry) noexcept
{
    // LOG_CONT entry: buffer raw message and flush complete lines.
    if (entry.loggerName[0] == CONT_ENTRY_MARKER)
    {
        size_t avail = CONT_BUF_SIZE - m_contBufLen;
        size_t copy  = std::min(entry.msgLen, avail);
        std::memcpy(m_contBuf + m_contBufLen, entry.msg, copy);
        m_contBufLen += copy;
        m_contLevel   = entry.level;
        FlushContLines(false);
        return;
    }

    // Regular entry: flush any pending CONT buffer first (preserves ordering).
    if (m_contBufLen > 0)
    {
        FlushContLines(true);
    }

    // spdlog sinks allocate heap memory for formatting (spdlog::memory_buf_t).
    // An std::bad_alloc thrown inside this noexcept function would call std::terminate().
    try
    {
        // Route to named logger if entry has a logger name, otherwise use default logger.
        // spdlog::get() is called here on the drain thread (non-RT), so mutex is safe.
        std::shared_ptr<spdlog::logger> target;
        if (entry.loggerName[0] != '\0')
        {
            target = spdlog::get(entry.loggerName);
        }

        if (!target)
        {
            target = m_logger;
        }

        if (!target)
        {
            return;
        }

        auto wall_ns = m_timebase.ToWall_ns(entry.timeStamp_ns);
        auto duration = std::chrono::nanoseconds(wall_ns);
        auto tp = spdlog::log_clock::time_point(std::chrono::duration_cast<spdlog::log_clock::duration>(duration));

        target->log(
            tp,
            spdlog::source_loc{},
            entry.level,
            spdlog::string_view_t(entry.msg, entry.msgLen)
        );
    }
    catch (...)
    {
        // Formatting failed (likely std::bad_alloc). Increment drop counter so
        // Terminate() reports the true number of lost entries.
        m_dropCount.fetch_add(1, std::memory_order_relaxed);
    }
}

void RtLog::LogRtCont(LogLevel lvl, const char *msg, size_t msgLen) noexcept
{
    if (!m_initialized.load(std::memory_order_acquire)) return;
    if (static_cast<int>(lvl) < m_level.load(std::memory_order_relaxed)) return;
    Entry entry;
    entry.level        = lvl;
    entry.timeStamp_ns = MonoNow_ns();
    entry.loggerName[0] = CONT_ENTRY_MARKER;
    entry.loggerName[1] = '\0';
    size_t len = std::min(msgLen, QueueType::MsgLen() - 1);
    std::memcpy(entry.msg, msg, len);
    entry.msg[len] = '\0';
    entry.msgLen   = len;
    Enqueue(entry);
}

void RtLog::FlushContLines(bool force) noexcept
{
    static constexpr size_t IND = RtLogConstant::DEFAULT_CONT_INDENT;
    if (!m_logger) return;

    size_t start = 0;
    while (start < m_contBufLen)
    {
        char *nl = static_cast<char *>(std::memchr(m_contBuf + start, '\n', m_contBufLen - start));

        size_t lineLen;
        bool   hadNl;
        if (nl)
        {
            lineLen = static_cast<size_t>(nl - (m_contBuf + start));
            hadNl   = true;
        }
        else if (force)
        {
            lineLen = m_contBufLen - start;
            hadNl   = false;
        }
        else
        {
            break;
        }

        // Build: 21-space indent + line body
        char ibuf[IND + CONT_BUF_SIZE + 1];
        std::memset(ibuf, ' ', IND);
        size_t copy = std::min(lineLen, CONT_BUF_SIZE);
        std::memcpy(ibuf + IND, m_contBuf + start, copy);
        size_t ilen = IND + copy;
        ibuf[ilen] = '\0';

        try
        {
            m_logger->set_pattern("%v");
            m_logger->log(m_contLevel, spdlog::string_view_t(ibuf, ilen));
            m_logger->set_pattern(m_patternStr);
        }
        catch (...)
        {
            // Formatting failed (likely std::bad_alloc). Increment drop counter so
            // Terminate() reports the true number of lost entries.
            m_dropCount.fetch_add(1, std::memory_order_relaxed);
        }

        start += lineLen + (hadNl ? 1 : 0);
    }

    // Slide remaining partial line to front
    if (start > 0)
    {
        size_t remaining = m_contBufLen - start;
        if (remaining > 0)
        {
            std::memmove(m_contBuf, m_contBuf + start, remaining);
        }
        m_contBufLen = remaining;
    }
}

void RtLog::Enqueue(const Entry &entry) noexcept
{
    // RT-safe: lock-free push only, no semaphore post, no syscall
    if (!m_queue.TryPush(entry))
    {
        // Only increment the drop counter; pushing into a full queue is pointless.
        // Monitor via drop_count() or display with TUI_SET_ROW_V.
        m_dropCount.fetch_add(1, std::memory_order_relaxed);
    }
}

bool RtLog::IsActiveLevel(LogLevel lvl) const noexcept
{
    return (static_cast<int>(lvl) >= m_level.load(std::memory_order_relaxed));
}

std::string RtLog::AnnotateFilenameDatetime(const std::string &fileBasename)
{
    spdlog::filename_t filename;

    time_t tnow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm now_tm = spdlog::details::os::localtime(tnow);

    auto [basename, ext] = spdlog::details::file_helper::split_by_extension(fileBasename);

    filename = fmt::format(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}{}"),
                           basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1,
                           now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ext);

    return filename;
}

std::tuple<std::string, std::string> RtLog::SplitByDirectory(const std::string &fname)
{
    auto dirIndex = fname.rfind('/');

    // no valid directory found - return empty string as folder and whole path
    if (dirIndex == std::string::npos)
    {
        return {std::string(), fname};
    }

    // ends up with '/' - return whole path as directory and empty string as filename
    if (dirIndex == fname.size() - 1)
    {
        return {fname, std::string()};
    }

    // finally - return a valid directory and file path tuple
    return {fname.substr(0, dirIndex + 1), fname.substr(dirIndex + 1)};   // '/' is included as directory name
}

bool RtLog::EnsureDirectoryExistes(const std::string &dname, std::error_code &ec)
{
    if (dname.empty())
    {
        return true;
    }

    struct stat st {};
    if (::stat(dname.c_str(), &st) == 0)
    {
        if (S_ISDIR(st.st_mode))
        {
            return true;
        }

        ec.assign(ENOTDIR, std::generic_category());
        return false;
    }

    if (::mkdir(dname.c_str(), 0755) == 0)
    {
        return true;
    }

    ec.assign(errno, std::generic_category());
    return false;
}

// ─── RtLog::LogRtStream ──────────────────────────────────────────────────────

RtLog::LogRtStream::LogRtStream(LogLevel lvl) noexcept
    : m_logLevel(lvl),
      m_active(Instance().IsInitialized() && Instance().IsActiveLevel(lvl)),
      m_pos(0)
{
    m_buf[0] = '\0';
}

RtLog::LogRtStream::~LogRtStream() noexcept {
    if (m_active && !m_submitted && m_pos > 0)
    {
        Instance().LogRt(m_logLevel, "%.*s", (int)m_pos, m_buf);
    }
}

void RtLog::LogRtStream::Append(const char *src, size_t len) noexcept
{
    size_t avail = BUF_LEN - 1 - m_pos;
    size_t copy  = (len < avail) ? len : avail;
    memcpy(m_buf + m_pos, src, copy);
    m_pos        += copy;
    m_buf[m_pos]  = '\0';
}

void RtLog::LogRtStream::AddTruncation() noexcept
{
    if (m_pos + 3 < BUF_LEN)
    {
        m_buf[m_pos++] = '.';
        m_buf[m_pos++] = '.';
        m_buf[m_pos++] = '.';
        m_buf[m_pos] = '\0';
    }
}

bool RtLog::LogRtStream::AddSeparator() noexcept
{
    if (m_pos + 2 < BUF_LEN)
    {
        m_buf[m_pos++] = ',';
        m_buf[m_pos++] = ' ';
        return true;
    }
    return false;
}

void RtLog::LogRtStream::CloseArray() noexcept
{
    if (m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = ']';
        m_buf[m_pos] = '\0';
    }
}

int RtLog::LogRtStream::FormatIntState(long long val, bool is_signed) noexcept
{
    char fmtbuf[20];
    char *p = fmtbuf;
    *p++ = '%';

    const int w = m_width;
    m_width = 0;  // one-shot: reset before snprintf

    if (w > 0 && m_fillChar == '0') *p++ = '0';
    if      (w >= 100) { *p++ = char('0' + w / 100); *p++ = char('0' + (w % 100) / 10); *p++ = char('0' + w % 10); }
    else if (w >=  10) { *p++ = char('0' + w / 10);  *p++ = char('0' + w % 10); }
    else if (w >    0) { *p++ = char('0' + w); }

    *p++ = 'l'; *p++ = 'l';
    *p++ = m_hexMode ? 'X' : (is_signed ? 'd' : 'u');
    *p   = '\0';

    if (m_hexMode || !is_signed)
    {
        return std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, fmtbuf,
                             static_cast<unsigned long long>(val));
    }
    return std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, fmtbuf, val);
}

// ─── RtLog::NamedLogRtStream ─────────────────────────────────────────────────

RtLog::NamedLogRtStream::NamedLogRtStream(const char *logName, LogLevel lvl) noexcept
    : m_logLevel(lvl),
      m_active(Instance().IsInitialized() && Instance().IsActiveLevel(lvl)),
      m_submitted(false),
      m_pos(0)
{
    strncpy(m_logName, logName, NAME_BUF_LEN - 1);
    m_logName[NAME_BUF_LEN - 1] = '\0';
    m_buf[0] = '\0';
}

RtLog::NamedLogRtStream::~NamedLogRtStream() noexcept
{
    if (m_active && !m_submitted && m_pos > 0)
    {
        Instance().LogRtNamed(m_logName, m_logLevel, "%.*s", (int)m_pos, m_buf);
    }
}

void RtLog::NamedLogRtStream::Append(const char *src, size_t len) noexcept
{
    size_t avail = BUF_LEN - 1 - m_pos;
    size_t copy  = (len < avail) ? len : avail;
    memcpy(m_buf + m_pos, src, copy);
    m_pos       += copy;
    m_buf[m_pos] = '\0';
}

void RtLog::NamedLogRtStream::AddTruncation() noexcept
{
    if (m_pos + 3 < BUF_LEN)
    {
        m_buf[m_pos++] = '.';
        m_buf[m_pos++] = '.';
        m_buf[m_pos++] = '.';
        m_buf[m_pos]   = '\0';
    }
}

bool RtLog::NamedLogRtStream::AddSeparator() noexcept
{
    if (m_pos + 2 < BUF_LEN)
    {
        m_buf[m_pos++] = ',';
        m_buf[m_pos++] = ' ';
        return true;
    }
    return false;
}

void RtLog::NamedLogRtStream::CloseArray() noexcept
{
    if (m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = ']';
        m_buf[m_pos]   = '\0';
    }
}

int RtLog::NamedLogRtStream::FormatIntState(long long val, bool is_signed) noexcept
{
    char fmtbuf[20];
    char *p = fmtbuf;
    *p++ = '%';
    const int w = m_width;
    m_width = 0;
    if (w > 0 && m_fillChar == '0') *p++ = '0';
    if      (w >= 100) { *p++ = char('0' + w / 100); *p++ = char('0' + (w % 100) / 10); *p++ = char('0' + w % 10); }
    else if (w >=  10) { *p++ = char('0' + w / 10);  *p++ = char('0' + w % 10); }
    else if (w >    0) { *p++ = char('0' + w); }
    *p++ = 'l'; *p++ = 'l';
    *p++ = m_hexMode ? 'X' : (is_signed ? 'd' : 'u');
    *p   = '\0';
    if (m_hexMode || !is_signed)
        return std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, fmtbuf, static_cast<unsigned long long>(val));
    return std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, fmtbuf, val);
}

// ─── RtLog::LogRtContStream ──────────────────────────────────────────────────

RtLog::LogRtContStream::LogRtContStream(LogLevel lvl) noexcept
    : m_logLevel(lvl),
      m_active(Instance().IsInitialized() && Instance().IsActiveLevel(lvl)),
      m_pos(0)
{
    m_buf[0] = '\0';
}

RtLog::LogRtContStream::~LogRtContStream() noexcept
{
    if (m_active && m_pos > 0)
    {
        Instance().LogRtCont(m_logLevel, m_buf, m_pos);
    }
}

void RtLog::LogRtContStream::Append(const char *src, size_t len) noexcept
{
    size_t avail = BUF_LEN - 1 - m_pos;
    size_t copy  = (len < avail) ? len : avail;
    std::memcpy(m_buf + m_pos, src, copy);
    m_pos       += copy;
    m_buf[m_pos] = '\0';
}

void RtLog::LogRtContStream::AddTruncation() noexcept
{
    if (m_pos + 3 < BUF_LEN)
    {
        m_buf[m_pos++] = '.';
        m_buf[m_pos++] = '.';
        m_buf[m_pos++] = '.';
        m_buf[m_pos]   = '\0';
    }
}

bool RtLog::LogRtContStream::AddSeparator() noexcept
{
    if (m_pos + 2 < BUF_LEN)
    {
        m_buf[m_pos++] = ',';
        m_buf[m_pos++] = ' ';
        return true;
    }
    return false;
}

void RtLog::LogRtContStream::CloseArray() noexcept
{
    if (m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = ']';
        m_buf[m_pos]   = '\0';
    }
}

int RtLog::LogRtContStream::FormatIntState(long long val, bool is_signed) noexcept
{
    char fmtbuf[20];
    char *p = fmtbuf;
    *p++ = '%';
    const int w = m_width;
    m_width = 0;
    if (w > 0 && m_fillChar == '0') *p++ = '0';
    if      (w >= 100) { *p++ = char('0' + w / 100); *p++ = char('0' + (w % 100) / 10); *p++ = char('0' + w % 10); }
    else if (w >=  10) { *p++ = char('0' + w / 10);  *p++ = char('0' + w % 10); }
    else if (w >    0) { *p++ = char('0' + w); }
    *p++ = 'l'; *p++ = 'l';
    *p++ = m_hexMode ? 'X' : (is_signed ? 'd' : 'u');
    *p   = '\0';
    if (m_hexMode || !is_signed)
        return std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, fmtbuf, static_cast<unsigned long long>(val));
    return std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, fmtbuf, val);
}

// ─── Non-template operator<< for LogRtStream ─────────────────────────────────

RtLog::LogRtStream &RtLog::LogRtStream::operator<<(const char* str) noexcept
{
    if (m_active && str)
    {
        Append(str, strnlen(str, BUF_LEN));
    }
    return *this;
}

RtLog::LogRtStream &RtLog::LogRtStream::operator<<(const std::string &str) noexcept
{
    if (m_active)
    {
        Append(str.c_str(), str.size());
    }
    return *this;
}

RtLog::LogRtStream &RtLog::LogRtStream::operator<<(char c) noexcept
{
    if (m_active && m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = c;
        m_buf[m_pos] = '\0';
    }
    return *this;
}

// printf() — formats directly into the queue Entry via log_rt_v(), no intermediate buffer.
// Single format pass (same cost as LOG_PRINTF).
RtLog::LogRtStream &RtLog::LogRtStream::printf(const char *fmt, ...) noexcept
{
    if (!m_active || !fmt)
    {
        return *this;
    }
    m_submitted = true;
    va_list args;
    va_start(args, fmt);
    Instance().LogRtV(m_logLevel, fmt, args);
    va_end(args);
    return *this;
}

// std::hex / std::dec stream manipulators — update hex mode flag.
// Other manipulators (std::oct, std::uppercase, …) are silently ignored.
RtLog::LogRtStream &RtLog::LogRtStream::operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept
{
    if (!m_active) return *this;
    if      (fn == std::hex) m_hexMode = true;
    else if (fn == std::dec) m_hexMode = false;
    return *this;
}

// std::setw(n) — GCC/libstdc++: returns std::_Setw{_M_n}.
RtLog::LogRtStream &RtLog::LogRtStream::operator<<(decltype(std::setw(0)) w) noexcept
{
    if (m_active) m_width = w._M_n;
    return *this;
}

// std::setfill(c) — GCC/libstdc++: returns std::_Setfill<char>{_M_c}.
// Only '0' and ' ' (default) take effect; other chars are accepted but fall back to space.
RtLog::LogRtStream &RtLog::LogRtStream::operator<<(decltype(std::setfill(' ')) f) noexcept
{
    if (m_active) m_fillChar = f._M_c;
    return *this;
}

// ─── Non-template operator<< for NamedLogRtStream ────────────────────────────

RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(const char *str) noexcept
{
    if (m_active && str)
    {
        Append(str, strnlen(str, BUF_LEN));
    }
    return *this;
}

RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(const std::string &str) noexcept
{
    if (m_active)
    {
        Append(str.c_str(), str.size());
    }
    return *this;
}

RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(char c) noexcept
{
    if (m_active && m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = c;
        m_buf[m_pos]   = '\0';
    }
    return *this;
}

RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept
{
    if (!m_active)
    {
        return *this;
    }

    if (fn == std::hex)
    {
        m_hexMode = true;
    }
    else if (fn == std::dec)
    {
        m_hexMode = false;
    }

    return *this;
}

RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(decltype(std::setw(0)) w) noexcept
{
    if (m_active)
    {
        m_width = w._M_n;
    }

    return *this;
}

RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(decltype(std::setfill(' ')) f) noexcept
{
    if (m_active)
    {
        m_fillChar = f._M_c;
    }

    return *this;
}

// printf-style: RT 큐를 통해 처리 (RT-safe)
RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::printf(const char *fmt, ...) noexcept
{
    if (!m_active || !fmt)
    {
        return *this;
    }
    m_submitted = true;
    va_list args;
    va_start(args, fmt);
    Instance().LogRtNamedV(m_logName, m_logLevel, fmt, args);
    va_end(args);
    return *this;
}

// ─── Non-template operator<< for LogRtContStream ─────────────────────────────

RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(const char *str) noexcept
{
    if (m_active && str) Append(str, strnlen(str, BUF_LEN));
    return *this;
}

RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(const std::string &str) noexcept
{
    if (m_active) Append(str.c_str(), str.size());
    return *this;
}

RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(char c) noexcept
{
    if (m_active && m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = c;
        m_buf[m_pos]   = '\0';
    }
    return *this;
}

RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept
{
    if (!m_active) return *this;
    if      (fn == std::hex) m_hexMode = true;
    else if (fn == std::dec) m_hexMode = false;
    return *this;
}

RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(decltype(std::setw(0)) w) noexcept
{
    if (m_active) m_width = w._M_n;
    return *this;
}

RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(decltype(std::setfill(' ')) f) noexcept
{
    if (m_active) m_fillChar = f._M_c;
    return *this;
}

RtLog::LogRtContStream &RtLog::LogRtContStream::printf(const char *fmt, ...) noexcept
{
    if (!m_active || !fmt) return *this;
    size_t avail = BUF_LEN - m_pos;
    if (avail == 0) return *this;
    va_list args;
    va_start(args, fmt);
    int n = std::vsnprintf(m_buf + m_pos, avail, fmt, args);
    va_end(args);
    if (n > 0) m_pos += std::min(static_cast<size_t>(n), avail - 1);
    m_buf[m_pos] = '\0';
    return *this;
}

}   // namespace Log
}   // namespace dt
