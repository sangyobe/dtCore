/*!
 \file      dtRtLog.hpp
 \brief     Logger for real-time system
 \author    myungjin.kim@hyundai.com
 \date      2026. 4. 24
 \version   0.0.2
 \copyright RoboticsLab ART All rights reserved.
*/
#ifndef _DT_RTLOG_H_
#define _DT_RTLOG_H_

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <sys/stat.h>
#include <atomic>
#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <type_traits>
#include <vector>
#include <iomanip>

#include "dtLogQueue.hpp"
#include "dtRtTui.hpp"

// Forward declaration for optional Eigen support (include dtRtLogEigen.hpp for the implementation)
namespace Eigen
{
    template<typename Derived>
    class MatrixBase;
}

// Forward declarations for optional dt::Math vector support (include dtRtLogDtMath.hpp for the implementation)
namespace dt
{
namespace Math
{
    template<uint16_t t_row, typename t_type> class Vector;
    template<typename t_type, uint16_t t_row> class Vector3;
    template<typename t_type, uint16_t t_row> class Vector4;
    template<typename t_type, uint16_t t_row> class Vector6;
    template<typename t_type>                 class VectorX;
}   // namespace Math
}   // namespace dt

namespace dt {
namespace Thread {
    struct _threadInfo;
    typedef struct _threadInfo ThreadInfo;
}
}

namespace dt
{

namespace Log
{

using LogLevel = spdlog::level::level_enum;

typedef uint32_t LogPattern;

class LogPatternFlag {
public:
    enum _flag {
        none         = 0,
        type         = 0x0001,
        type_long    = 0x0002,
        date         = 0x0010,
        time         = 0x0020,
        datetime     = 0x0040,
        epoch        = 0x0080,
        elapsed      = 0x0100,
        name         = 0x0004,
    };
};
namespace RtLogConstant
{
    inline constexpr size_t DEFAULT_MAX_SIZE    = 10 * 1024 * 1024;  // 10MB
    inline constexpr size_t DEFAULT_MAX_FILES   = 5;
    inline constexpr size_t INTERNAL_BUF_SIZE   = 65536;  // 64 KB internal buffer
    inline constexpr size_t QUEUE_CAPACITY      = 1024;
    inline constexpr size_t QUEUE_MSGLEN        = 1024;
    // Maximum delay between log output bursts (nanoseconds)
    inline constexpr long POLL_INTERVAL_NS      = 1'000'000L; // 1 ms
    // Thread info
    inline constexpr size_t THREAD_STACK_SIZE   = 1024 * 1024; // 1MB
    inline constexpr int THREAD_CPU_ID          = 2;  // default CPU core(#2)
    inline constexpr int THREAD_PRIORITY        = 0;  // nonRt
    // Visible prefix width of pattern "%^[%L][%H:%M:%S.%f]%$ %v":
    // "[I]"=3 + "[HH:MM:SS.ffffff]"=17 + " "=1 = 21 chars
    inline constexpr size_t DEFAULT_CONT_INDENT = 21;
}   // namespace RtLogConstant

// Colored stdout sink using a single write() syscall per message.
//
// All LOG_RT() entries are produced by RT tasks into the MPSC queue and
// consumed exclusively by the single drain thread. Using write() directly
// (rather than rt_printf) avoids Xenomai's per-task ring buffer, which
// overflows during burst drains (72+ messages at once) and causes truncation.
// A single write() call for a complete formatted string is effectively atomic
// on a tty for message sizes well within PIPE_BUF (4096 bytes).
//
// Shared level → ANSI color mapping used by ColorStdoutSinkT and TuiSinkT
inline const char *SinkColorFor(spdlog::level::level_enum lvl) noexcept
{
    switch (lvl)
    {
        case spdlog::level::trace:    return "\033[37m";
        case spdlog::level::debug:    return "\033[36m";
        case spdlog::level::info:     return "\033[32m";
        case spdlog::level::warn:     return "\033[33m\033[1m";
        case spdlog::level::err:      return "\033[31m\033[1m";
        case spdlog::level::critical: return "\033[1m\033[41m";
        default:                      return "";
    }
}

// ColorStdoutSinkT — colored stdout sink with internal write buffer
//
// Design:
//   sink_it_(): formats to internal buffer only (no write() syscall)
//   flush_()  : writes buffer to stdout via a single O_NONBLOCK write()
//               EAGAIN when pty is full → skip current frame (no blocking)
//
// drain_all() can process 2000+ messages without any write() syscall, so the
// drain thread never blocks on pty I/O.
// Safe on Xenomai even when the SCHED_OTHER terminal emulator drains the pty slowly.
//
// Two aliases:
//   ColorStdoutSink   — null_mutex, drain thread (single consumer, no contention)
//   ColorStdoutSinkMt — std::mutex, main logger (may be called from multiple threads)
template<typename Mutex>
class ColorStdoutSinkT final : public spdlog::sinks::base_sink<Mutex>
{
    using Base = spdlog::sinks::base_sink<Mutex>;

public:
    ColorStdoutSinkT()
    {
        // Open a new open file description for stdout via /proc/self/fd/1.
        // O_NONBLOCK is set only on this private fd — STDOUT_FILENO is unaffected,
        // so printf/cout/other-sinks never see EAGAIN.
        // O_APPEND ensures atomic-seek-to-EOF on each write(), preventing offset
        // corruption when stdout is redirected to a regular file.
        m_stdout_fd = ::open("/proc/self/fd/1", O_WRONLY | O_APPEND | O_NONBLOCK | O_CLOEXEC);
        if (m_stdout_fd < 0)
        {
            m_stdout_fd = STDOUT_FILENO;  // /proc unavailable: fall back to blocking writes
        }
    }

    ~ColorStdoutSinkT() override
    {
        // Switch the private fd to blocking so the final flush always completes.
        // This fcntl affects only m_stdout_fd; STDOUT_FILENO is untouched.
        if (m_stdout_fd != STDOUT_FILENO)
        {
            int flags = ::fcntl(m_stdout_fd, F_GETFL, 0);
            if (flags != -1)
            {
                ::fcntl(m_stdout_fd, F_SETFL, flags & ~O_NONBLOCK);
            }
        }

        FlushBuffer();

        if (m_stdout_fd != STDOUT_FILENO)
        {
            ::close(m_stdout_fd);
        }
    }

    ColorStdoutSinkT(const ColorStdoutSinkT &)            = delete;
    ColorStdoutSinkT &operator=(const ColorStdoutSinkT &) = delete;

protected:
    void sink_it_(const spdlog::details::log_msg &msg) override
    {
        // Reset before formatting so the formatter's %^ / %$ handlers always
        // write fresh values (mirrors what ansicolor_sink does in log()).
        msg.color_range_start = 0;
        msg.color_range_end   = 0;

        spdlog::memory_buf_t buf;
        Base::formatter_->format(msg, buf);

        const char *color = SinkColorFor(msg.level);
        if (*color && msg.color_range_end > msg.color_range_start)
        {
            AppendData(buf.data(), msg.color_range_start);
            AppendData(color, std::strlen(color));
            AppendData(buf.data() + msg.color_range_start, msg.color_range_end - msg.color_range_start);
            const char *reset_color = "\033[m";
            AppendData(reset_color, std::strlen(reset_color));
            AppendData(buf.data() + msg.color_range_end, buf.size() - msg.color_range_end);
        }
        else {
            AppendData(buf.data(), buf.size());
        }
    }

    void flush_() override
    {
        FlushBuffer();
    }

private:
    std::array<char, RtLogConstant::INTERNAL_BUF_SIZE> m_buf{};
    size_t m_pos{0};
    int m_stdout_fd{-1};

private:
    void AppendData(const char* data, size_t len) noexcept
    {
        if (len == 0)
        {
            return;
        }

        // Establish invariant: len < INTERNAL_BUF_SIZE.
        // For messages larger than the buffer, keep only the tail (most recent bytes).
        if (len >= RtLogConstant::INTERNAL_BUF_SIZE)
        {
            data += len - (RtLogConstant::INTERNAL_BUF_SIZE - 1);
            len   = RtLogConstant::INTERNAL_BUF_SIZE - 1;
        }

        // Make room: flush, then slide out oldest retained bytes when EAGAIN kept data.
        // If EAGAIN left data: slide guarantees m_pos + len == INTERNAL_BUF_SIZE.
        // If flush succeeded: m_pos == 0, inner if is skipped.
        if (m_pos + len > RtLogConstant::INTERNAL_BUF_SIZE)
        {
            FlushBuffer();
            if (m_pos + len > RtLogConstant::INTERNAL_BUF_SIZE)
            {
                size_t discard = m_pos + len - RtLogConstant::INTERNAL_BUF_SIZE;
                std::memmove(m_buf.data(), m_buf.data() + discard, m_pos - discard);
                m_pos -= discard;
            }
        }

        std::memcpy(m_buf.data() + m_pos, data, len);
        m_pos += len;
    }

    void FlushBuffer() noexcept
    {
        if (m_pos == 0)
        {
            return;
        }

        // O_NONBLOCK on m_stdout_fd: write() returns EAGAIN immediately when the pty
        // buffer is full → no drain thread blocking; unwritten data slides to the front
        // for next flush. STDOUT_FILENO is unaffected (remains blocking).
        size_t written = 0;
        while (written < m_pos)
        {
            ssize_t n = ::write(m_stdout_fd, m_buf.data() + written, m_pos - written);
            if (n < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                break;  // EAGAIN / EWOULDBLOCK: retry remaining bytes on next flush
            }

            if (n == 0)
            {
                break;
            }

            written += static_cast<size_t>(n);
        }

        // fully written — fast path
        if (written >= m_pos)
        {
            m_pos = 0;
            return;
        }

        // Slide unwritten bytes to buffer front
        size_t remaining = m_pos - written;
        if (remaining > 0 && written > 0)
        {
            std::memmove(m_buf.data(), m_buf.data() + written, remaining);
        }

        m_pos = remaining;
    }
};

using ColorStdoutSink   = ColorStdoutSinkT<spdlog::details::null_mutex>;
using ColorStdoutSinkMt = ColorStdoutSinkT<std::mutex>;

// TUI Sink: Routes spdlog messages to RtTui instead of stdout
// This sink is used when TUI mode is enabled (enableTui: true in config.yaml)
// Messages are sent to TUI's log ring buffer for display in Area 2
template<typename Mutex>
class TuiSinkT final : public spdlog::sinks::base_sink<Mutex>
{
    using Base = spdlog::sinks::base_sink<Mutex>;

public:
    explicit TuiSinkT(std::shared_ptr<Log::RtTui> tui) : m_tui(tui) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}

private:
    std::shared_ptr<Log::RtTui> m_tui;
};

using TuiSink   = TuiSinkT<spdlog::details::null_mutex>;
using TuiSinkMt = TuiSinkT<std::mutex>;

// File sink using buffered writes with periodic flush and rotation support.
//
// Similar to ColorStdoutSinkT but writes to a file with internal buffering.
// Unlike unbuffered write(), this sink accumulates messages in a buffer and
// flushes periodically or when the buffer is full, reducing syscall overhead
// and improving performance in high-throughput logging scenarios.
//
// Key features:
// - Internal buffer (default 64KB) to batch multiple log messages
// - Automatic flush when buffer reaches threshold
// - Manual flush via flush_() for critical messages
// - Non-blocking writes (kernel buffering) for better performance
// - File rotation based on size limit
// - Configurable number of rotated files to keep
//
// Two aliases:
//   BasicFileSink   — null_mutex, single-threaded usage
//   BasicFileSinkMt — std::mutex, multi-threaded usage
template<typename Mutex>
class BasicFileSinkT final : public spdlog::sinks::base_sink<Mutex>
{
    using Base = spdlog::sinks::base_sink<Mutex>;

public:
    explicit BasicFileSinkT(const std::string& filename, size_t max_size, size_t max_files, bool truncate = false)
        : m_fd(-1),
          m_baseFilename(filename),
          m_bufPos(0),
          m_currentSize(0),
          m_maxSize(max_size),
          m_maxFiles(max_files == 0 ? 1 : max_files)
    {
        OpenFile(truncate);
    }

    ~BasicFileSinkT() override
    {
        if (m_fd >= 0)
        {
            FlushBuffer();
            ::close(m_fd);
        }
    }

    // Delete copy and move
    BasicFileSinkT(const BasicFileSinkT &) = delete;
    BasicFileSinkT &operator=(const BasicFileSinkT &) = delete;
    BasicFileSinkT(BasicFileSinkT &&) = delete;
    BasicFileSinkT &operator=(BasicFileSinkT &&) = delete;

    const std::string &filename() const noexcept
    {
        return m_baseFilename;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override
    {
        if (m_fd < 0)
        {
            return;
        }

        spdlog::memory_buf_t buf;
        Base::formatter_->format(msg, buf);

        // Check if rotation is needed
        if (m_maxSize > 0 && m_currentSize + m_bufPos + buf.size() > m_maxSize)
        {
            RotateFiles();
        }

        // If message is larger than buffer, flush current buffer and write directly
        if (buf.size() > RtLogConstant::INTERNAL_BUF_SIZE)
        {
            FlushBuffer();
            size_t total = 0;
            while (total < buf.size())
            {
                ssize_t n = ::write(m_fd, buf.data() + total, buf.size() - total);
                if (n <= 0)
                {
                    if (errno == EINTR)
                    {
                        continue;
                    }

                    break;
                }

                total += n;
            }
            m_currentSize += total;
            return;
        }

        // If adding this message would overflow buffer, flush first
        if (m_bufPos + buf.size() > RtLogConstant::INTERNAL_BUF_SIZE)
        {
            FlushBuffer();
        }

        // Append to buffer
        std::memcpy(m_buffer.data() + m_bufPos, buf.data(), buf.size());
        m_bufPos += buf.size();
    }

    void flush_() override
    {
        FlushBuffer();
    }

private:
    int m_fd;
    std::string m_baseFilename;
    std::array<char, RtLogConstant::INTERNAL_BUF_SIZE> m_buffer{};
    size_t m_bufPos;
    size_t m_currentSize;
    size_t m_maxSize;
    size_t m_maxFiles;

private:
    void OpenFile(bool truncate) noexcept
    {
        int flags = O_WRONLY | O_CREAT | O_CLOEXEC;
        if (truncate)
        {
            flags |= O_TRUNC;
            m_currentSize = 0;
        }
        else
        {
            flags |= O_APPEND;
            // Get current file size
            struct stat st;
            if (::stat(m_baseFilename.c_str(), &st) == 0)
            {
                m_currentSize = st.st_size;
            }
            else
            {
                m_currentSize = 0;
            }
        }

        m_fd = ::open(m_baseFilename.c_str(), flags, 0644);
        if (m_fd < 0)
        {
            static const char kOpenErr[] = "[RtLog] failed to open log file\n";
            ssize_t n = ::write(STDERR_FILENO, kOpenErr, sizeof(kOpenErr) - 1);
            if (n < 0) {}   // NOP: write error is occurred but it don't needed to be reported
        }
    }

    void FlushBuffer() noexcept
    {
        if (m_fd < 0 || m_bufPos == 0)
        {
            return;
        }

        size_t total_written = 0;
        while (total_written < m_bufPos)
        {
            ssize_t written = ::write(m_fd, m_buffer.data() + total_written, m_bufPos - total_written);
            if (written < 0)
            {
                if (errno == EINTR)
                {
                    continue;  // Interrupted by signal - retry
                }
                // Other error: discard buffer to avoid blocking.
                // Notify via stderr (safe from noexcept, no allocation).
                static const char kWriteErr[] = "[RtLog] file write error, log data lost\n";
                ssize_t n = ::write(STDERR_FILENO, kWriteErr, sizeof(kWriteErr) - 1);
                if (n < 0) {}   // NOP: write error is occurred but it don't needed to be reported
                break;
            }
            else if (written == 0)
            {
                // Disk full or quota exceeded - discard buffer
                static const char kDiskFull[] = "[RtLog] disk full, log data lost\n";
                ssize_t n = ::write(STDERR_FILENO, kDiskFull, sizeof(kDiskFull) - 1);
                if (n < 0) {}   // NOP: write error is occurred but it don't needed to be reported
                break;
            }

            total_written += written;
        }

        // Update current size with what was actually written
        if (total_written > 0)
        {
            m_currentSize += total_written;
        }

        m_bufPos = 0;
    }

    void RotateFiles() noexcept
    {
        // Flush and close current file
        FlushBuffer();
        if (m_fd >= 0)
        {
            ::close(m_fd);
            m_fd = -1;
        }

        // std::string operations can throw std::bad_alloc.
        // Wrap in try-catch so that a noexcept boundary violation cannot trigger std::terminate().
        try
        {
            for (size_t i = m_maxFiles - 1; i > 0; --i)
            {
                std::string src = m_baseFilename + "." + std::to_string(i);
                std::string dst = m_baseFilename + "." + std::to_string(i + 1);
                if (i == m_maxFiles - 1)
                {
                    ::unlink(dst.c_str());
                }
                ::rename(src.c_str(), dst.c_str());
            }
            std::string backup = m_baseFilename + ".1";
            ::rename(m_baseFilename.c_str(), backup.c_str());
        }
        catch (...)
        {
            // Allocation failure: skip rename chain, just open a new (truncated) file
        }

        // Open new file
        OpenFile(true);
    }
};

using BasicFileSink   = BasicFileSinkT<spdlog::details::null_mutex>;
using BasicFileSinkMt = BasicFileSinkT<std::mutex>;

class RtLog {
public:
    // Increased capacity from 256 to 2048 to handle high-frequency burst logging
    // if system generates total message per ~3400 msg/sec
    // With 2048 capacity, can buffer ~600ms worth of messages during drain delays
    using QueueType = LogQueue<RtLogConstant::QUEUE_CAPACITY, RtLogConstant::QUEUE_MSGLEN>;
    using Entry = QueueType::Entry;

    struct TimeBase
    {
        int64_t wall_ns;      // CLOCK_REALTIME
        int64_t monotonic_ns; // CLOCK_MONOTONIC — must match the clock used in MonoNow_ns()

        // need to be called in nonRT before log task is starting
        static TimeBase Capture() noexcept
        {
            TimeBase tb{};
            struct timespec tw{}, tm{};
            clock_gettime(CLOCK_REALTIME, &tw);
            clock_gettime(CLOCK_MONOTONIC, &tm);  // must match MonoNow_ns()
            tb.wall_ns = static_cast<int64_t>(tw.tv_sec) * 1'000'000'000LL + tw.tv_nsec;
            tb.monotonic_ns = static_cast<int64_t>(tm.tv_sec) * 1'000'000'000LL + tm.tv_nsec;
            return tb;
        }

        // convert monotonic to wall clock
        int64_t ToWall_ns(int64_t mono_ns) const noexcept {
            return wall_ns + (mono_ns - monotonic_ns);
        }
    };

public:
    static RtLog &Instance() noexcept
    {
        static RtLog m_instance;
        return m_instance;
    }

    static void Initialize(
        const std::string &logName,
        const std::string &fileBasename = "",
        bool enableTui = false,
        int threadCpuId = RtLogConstant::THREAD_CPU_ID,
        size_t maxFiles = RtLogConstant::DEFAULT_MAX_FILES,
        size_t maxFileSize = RtLogConstant::DEFAULT_MAX_SIZE,
        int threadPriority = RtLogConstant::THREAD_PRIORITY,
        size_t threadStack = RtLogConstant::THREAD_STACK_SIZE,
        bool annotDatetime = true,
        bool truncate = false);

    /**
     * Default logger 외에 새로운 로거를 생성하고 spdlog 레지스트리에 등록.
     * @param logName logger 이름.
     * @param fileBasename 로그 파일 이름. "_STDOUT_"인 경우 terminal. 그 외는 해당 파일명으로 로그 생성.
     * @param annotDatetime 파일 로그의 경우 파일 이름에 생성 날짜 및 시간을 뒤에 붙일지 여부.
     * @param truncate 동일 이름의 로그 파일이 있는 경우 해당 파일을 지우고 새로 만들지 여부.
     * @param maxFiles 최대 로그 파일 개수 (파일 rotation 시).
     * @param maxFileSize 최대 파일 크기 (bytes).
     */
    static void Create(
        const std::string &logName,
        const std::string &fileBasename,
        bool annotDatetime = true,
        bool truncate = false,
        size_t maxFiles = RtLogConstant::DEFAULT_MAX_FILES,
        size_t maxFileSize = RtLogConstant::DEFAULT_MAX_SIZE);

    /**
     * Terminate logging system
     */
    static void Terminate();

    // dummy function for migration
    static void FlushOn(LogLevel lvl);
    static void FlushOn(const std::string &logger_name, LogLevel lvl);

    /**
     * Set log level of default logger
     * @param lvl log level
     */
    static void SetLogLevel(LogLevel lvl);
    static void SetLogLevel(const std::string &logger_name, LogLevel lvl);

    /**
     * Default logger 로그 패턴 설정 (flag 조합 방식).
     * @param pattern LogPatternFlag 조합.
     * @param delimiter 각 항목 사이의 구분자 문자열 (e.g. "|", " ").
     *
     * 주의: spdlog 원시 패턴 문자열을 직접 사용하려면
     *       SetLogPattern(const std::string &raw_pattern) 오버로드를 사용하세요.
     */
    static void SetLogPattern(LogPattern pattern = LogPatternFlag::type|LogPatternFlag::time, const std::string &delimiter = "|");
    static void SetLogPattern(const std::string &logger_name, LogPattern pattern, const std::string &delimiter = "|");

    /**
     * Default logger 로그 패턴 설정 (spdlog 원시 패턴 문자열 방식).
     * @param raw_pattern spdlog 패턴 문자열 (e.g. "[%Y-%m-%d %H:%M:%S.%f][%^%l%$] %n: %v").
     */
    static void SetLogPattern(const std::string &raw_pattern);
    static void SetLogPattern(const std::string &logger_name, const std::string &raw_pattern);

    // Returns the TUI instance (nullptr when TUI is disabled)
    std::shared_ptr<Log::RtTui> GetTui() const noexcept;

    // Get the latest pending key from TUI (non-blocking, returns 0 if no key)
    char GetPendingKey() const noexcept;

    // TUI Area 1 helpers — RT-safe no-ops when TUI is disabled
    // layoutIdx: 0 ~ MAX_LAYOUTS-1  groupIdx: 0 ~ TUI_MAX_GROUPS-1  rowIdx: 0 ~ TUI_MAX_ROWS_PER_GROUP-1
    template<typename... Args>
    void TuiSetRow(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *format, Args... args) noexcept
    {
        if (m_tui)
        {
            m_tui->SetRowFmt(layoutIdx, groupIdx, rowIdx, label, format, args...);
        }
    }

    template<typename... Cols>
    void TuiSetRowCols(int layoutIdx, int groupIdx, int rowIdx, const char *label, Cols &&...cols) noexcept
    {
        if (m_tui)
        {
            m_tui->SetRowCols(layoutIdx, groupIdx, rowIdx, label, std::forward<Cols>(cols)...);
        }
    }

    template<typename... Args>
    void TuiSetGroup(int layoutIdx, int groupIdx, const char *label, Args... args) noexcept
    {
        if (m_tui)
        {
            m_tui->SetGroupV(layoutIdx, groupIdx, label, args...);
        }
    }

    void TuiSetGroupNoHeader(int layoutIdx, int groupIdx) noexcept;
    void TuiSetTextRow(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *text) noexcept;
    void TuiSetTextRowFmt(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *fmt, ...) noexcept
        __attribute__((format(printf, 6, 7)));
    void TuiSetLayoutName(int layoutIdx, const char *name) noexcept;
    bool IsInitialized() const noexcept;
    void RefreshTimebase() noexcept;

    // Called repeatedly by the log thread in a loop.
    // Sleeps POLL_INTERVAL_NS then drains all queued entries.
    // No syscall occurs in the RT producer path.
    static void *PollLogQueue(void *pArg) noexcept;

    // Flush all remaining entries from the RT queue.
    // IMPORTANT: must only be called from a single consumer thread at a time.
    // LogQueue is MPSC — concurrent try_pop() from two threads is undefined behaviour.
    // Callers must ensure the drain thread has stopped before calling this (e.g. in Terminate()).
    size_t DrainAll() noexcept;

    // Wait until the drain thread has processed all messages currently in the queue.
    //
    // If called before format-affecting changes such as SetLogPattern() / SetLogLevel(),
    // this can prevent the issue where already-queued messages are output with the new pattern.
    //
    // How it works:
    //   1. Increment m_syncSeq by 1 to register a synchronization request with the drain thread.
    //   2. The drain thread responds by setting m_syncAck = m_syncSeq immediately after DrainAll() completes.
    //   3. Wait at 500μs intervals until m_syncAck >= target.
    //   4. After waiting, call flush() on the sink's internal buffer so that previous messages are reflected in the output device.
    //
    // Can only be used in a non-RT context (uses clock_nanosleep).
    static void Sync() noexcept;

    // Immediate raw output to STDERR, bypassing the drain thread and log queue.
    //
    // When to use (LOG_RT_RAW):
    //   - Drain thread not yet running (early init) or already stopped (shutdown)
    //   - Drain thread itself is the error source (self-reporting failures)
    //   - Critical system errors that must appear even if the queue is full / dropped
    //   - Signal handlers (sigdebug_handler, SIGINT, etc.)
    //
    // RT properties:
    //   - Stack-only: no heap allocation, no mutex, no queue
    //   - write(STDERR_FILENO) is a Linux syscall → causes Xenomai secondary-mode
    //     switch on Cobalt threads. Acceptable for error/panic paths.
    //   - Single write() ≤ PIPE_BUF (4096 B) is atomic on Linux: concurrent
    //     callers from different threads will not interleave mid-message.
    //   - clock_gettime(CLOCK_REALTIME) is Cobalt-intercepted: stays in primary mode.
    //   - Time shown is UTC (avoids localtime_r() which may malloc/lock).
    static void LogRaw(LogLevel lvl, const char *fmt, ...) noexcept __attribute__((format(printf, 2, 3)));

    template<typename... Args>
    void LogRt(LogLevel lvl, const char *format, Args... args) noexcept
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
        entry.Set(lvl, MonoNow_ns(), format, args...);
        Enqueue(entry);
    }

    void LogRtV(LogLevel lvl, const char* format, va_list args) noexcept;

    template<typename... Args>
    void LogRtFmt(LogLevel lvl, fmt::format_string<Args...> fmt_str, Args&&... args) noexcept
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
        entry.level        = lvl;
        entry.timeStamp_ns = MonoNow_ns();
        entry.loggerName[0] = '\0';
        try
        {
            static constexpr size_t cap = QueueType::MsgLen() - 1;
            auto result      = fmt::format_to_n(entry.msg, cap, fmt_str, std::forward<Args>(args)...);
            size_t sz        = result.size < cap ? result.size : cap;
            entry.msg[sz]    = '\0';
            entry.msgLen    = sz;
        }
        catch (...)
        {
            entry.msg[0]  = '\0';
            entry.msgLen = 0;
        }

        Enqueue(entry);
    }

    // Named logger path — called from LOG_U() destructor. Processed by the drain thread via the RT queue.
    template<typename... Args>
    void LogRtNamed(const char *loggerName, LogLevel lvl, const char *format, Args... args) noexcept
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
        entry.Set(lvl, MonoNow_ns(), format, args...);
        strncpy(entry.loggerName, loggerName, sizeof(entry.loggerName) - 1);
        entry.loggerName[sizeof(entry.loggerName) - 1] = '\0';
        Enqueue(entry);
    }

    void LogRtNamedV(const char *loggerName, LogLevel lvl, const char *format, va_list args) noexcept;

    template<typename... Args>
    void LogRtNamedFmt(const char *loggerName, LogLevel lvl, fmt::format_string<Args...> fmt_str, Args&&... args) noexcept
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
        entry.level        = lvl;
        entry.timeStamp_ns = MonoNow_ns();
        strncpy(entry.loggerName, loggerName, sizeof(entry.loggerName) - 1);
        entry.loggerName[sizeof(entry.loggerName) - 1] = '\0';
        try
        {
            static constexpr size_t cap = QueueType::MsgLen() - 1;
            auto result   = fmt::format_to_n(entry.msg, cap, fmt_str, std::forward<Args>(args)...);
            size_t sz     = result.size < cap ? result.size : cap;
            entry.msg[sz] = '\0';
            entry.msgLen  = sz;
        }
        catch (...)
        {
            entry.msg[0]  = '\0';
            entry.msgLen  = 0;
        }
        Enqueue(entry);
    }

    void SetLevel(LogLevel lvl) noexcept;
    LogLevel GetLevel() const noexcept;
    uint64_t DropCount() const noexcept;

    // Get approximate number of messages currently in queue
    size_t QueueSize() const noexcept;

    // Get queue utilization percentage (0-100)
    size_t QueueUtilization() const noexcept;

    // Get queue statistics
    struct QueueStats
    {
        size_t current_size;      // Current number of messages in queue
        size_t capacity;          // Maximum queue capacity
        size_t utilization_pct;   // Utilization percentage (0-100)
        uint64_t total_drops;     // Total number of dropped messages
    };

    QueueStats GetQueueStats() const noexcept;

    class [[nodiscard]] LogRtStream
    {
    public:
        static constexpr size_t BUF_LEN = QueueType::MsgLen();

        LogRtStream(LogLevel lvl) noexcept;

        // prevent copy and move
        LogRtStream(const LogRtStream &)            = delete;
        LogRtStream &operator=(const LogRtStream &) = delete;
        LogRtStream(LogRtStream &&)                 = delete;
        LogRtStream &operator=(LogRtStream &&)      = delete;

        ~LogRtStream() noexcept;

        template<typename T, typename = std::enable_if_t<
            std::is_arithmetic<T>::value ||
            std::is_pointer<T>::value ||
            std::is_enum<T>::value>>
        LogRtStream &operator<<(T value) noexcept;

        template <typename T>
        LogRtStream &operator<<(const std::vector<T> &value) noexcept;

        // Eigen dense vector/matrix support — implementation in dtRtLogEigen.hpp
        // Include dtRtLogEigen.hpp (not this header) in files that use Eigen types.
        template<typename Derived>
        LogRtStream &operator<<(const Eigen::MatrixBase<Derived> &vec) noexcept;

        // dt::Math vector support — implementation in dtRtLogDtMath.hpp
        // Include dtRtLogDtMath.hpp (not this header) in files that use dt::Math vectors.
        template<uint16_t N, typename T>
        LogRtStream &operator<<(const Math::Vector<N, T> &vec) noexcept;
        template<typename T, uint16_t N>
        LogRtStream &operator<<(const Math::Vector3<T, N> &vec) noexcept;
        template<typename T, uint16_t N>
        LogRtStream &operator<<(const Math::Vector4<T, N> &vec) noexcept;
        template<typename T, uint16_t N>
        LogRtStream &operator<<(const Math::Vector6<T, N> &vec) noexcept;
        template<typename T>
        LogRtStream &operator<<(const Math::VectorX<T> &vec) noexcept;

        LogRtStream &operator<<(const char *str) noexcept;
        LogRtStream &operator<<(const std::string &str) noexcept;
        LogRtStream &operator<<(char c) noexcept;
        LogRtStream &operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept;
        LogRtStream &operator<<(decltype(std::setw(0)) w) noexcept;
        LogRtStream &operator<<(decltype(std::setfill(' ')) f) noexcept;

        // printf-style append into the stream buffer.
        // Example: LOG(info).printf("x=%.3f idx=%d", x, idx);
        LogRtStream &printf(const char *fmt, ...) noexcept __attribute__((format(printf, 2, 3)));

        // fmt-style format() — formats directly into the queue Entry, no intermediate buffer.
        // Single format pass (same cost as LOG_PRINTF).
        // dt::Math::Vector / Eigen types have no fmt::formatter: use operator<< instead.
        template<typename... Args>
        LogRtStream &format(fmt::format_string<Args...> fmtStr, Args&&... args) noexcept {
            if (!m_active)
            {
                return *this;
            }
            m_submitted = true;
            Instance().LogRtFmt(m_logLevel, fmtStr, std::forward<Args>(args)...);
            return *this;
        }

    private:
        LogLevel m_logLevel;
        bool m_active;
        bool m_submitted{false};
        size_t m_pos;
        char m_buf[BUF_LEN];
        bool m_hexMode{false};
        int  m_width{0};
        char m_fillChar{' '};

    private:
        void Append(const char *src, size_t len) noexcept;

        // Helper function to format a single element to m_buf
        // Returns true if successful, false if buffer is full
        // Automatically adds null terminator after formatting
        template <typename T>
        bool FormatElement(T value) noexcept
        {
            // Ensure we have space for at least "..." if truncation needed
            if (m_pos + 10 >= BUF_LEN)
            {
                return false;
            }

            int written = 0;
            if constexpr (std::is_pointer_v<T>)
            {
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%p", static_cast<const void*>(value));
            }
            else if constexpr (std::is_enum_v<T>)
            {
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%lld", static_cast<long long>(static_cast<std::underlying_type_t<T>>(value)));
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%s", value ? "true" : "false");
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%.6f", static_cast<double>(value));
            }
            else if constexpr (std::is_signed_v<T>)
            {
                written = FormatIntState(static_cast<long long>(value), true);
            }
            else
            {
                written = FormatIntState(static_cast<long long>(static_cast<unsigned long long>(value)), false);
            }

            // Check if snprintf failed or buffer was insufficient
            if (written < 0 || written >= static_cast<int>(BUF_LEN - m_pos))
            {
                return false;
            }

            m_pos += written;

            // Always maintain null terminator
            if (m_pos < BUF_LEN)
            {
                m_buf[m_pos] = '\0';
            }

            return true;
        }

        // Helper to add truncation indicator
        void AddTruncation() noexcept;

        // Helper to add separator ", "
        bool AddSeparator() noexcept;

        // Helper to close array with ']'
        void CloseArray() noexcept;

        // Format integer with current hex/width/fill state (via m_hexMode, m_width, m_fillChar).
        // m_width is one-shot (reset after use). m_hexMode and m_fillChar are sticky.
        // Supports fill chars '0' and ' ' only (snprintf limitation).
        int FormatIntState(long long val, bool is_signed) noexcept;
    };

    // NamedLogRtStream: Create()로 생성한 named logger에 기록하는 RT-safe 스트림 인터페이스.
    // LOG()와 동일하게 MPSC 큐를 통해 drain 스레드에서 처리하므로 RT 태스크에서 사용 가능.
    // spdlog::get()은 drain 스레드(비RT)의 FlushEntry()에서만 호출됨.
    class [[nodiscard]] NamedLogRtStream
    {
    public:
        static constexpr size_t BUF_LEN      = QueueType::MsgLen();
        static constexpr size_t NAME_BUF_LEN = 64;

        explicit NamedLogRtStream(const char *logName, LogLevel lvl) noexcept;

        NamedLogRtStream(const NamedLogRtStream &)            = delete;
        NamedLogRtStream &operator=(const NamedLogRtStream &) = delete;
        NamedLogRtStream(NamedLogRtStream &&)                 = delete;
        NamedLogRtStream &operator=(NamedLogRtStream &&)      = delete;

        ~NamedLogRtStream() noexcept;

        template<typename T, typename = std::enable_if_t<
            std::is_arithmetic<T>::value ||
            std::is_pointer<T>::value ||
            std::is_enum<T>::value>>
        NamedLogRtStream &operator<<(T value) noexcept;

        template<typename T>
        NamedLogRtStream &operator<<(const std::vector<T> &value) noexcept;

        NamedLogRtStream &operator<<(const char *str) noexcept;
        NamedLogRtStream &operator<<(const std::string &str) noexcept;
        NamedLogRtStream &operator<<(char c) noexcept;
        NamedLogRtStream &operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept;
        NamedLogRtStream &operator<<(decltype(std::setw(0)) w) noexcept;
        NamedLogRtStream &operator<<(decltype(std::setfill(' ')) f) noexcept;

        // printf-style: RT 큐를 통해 처리 (RT-safe)
        NamedLogRtStream &printf(const char *fmt, ...) noexcept __attribute__((format(printf, 2, 3)));

        // fmt-style: RT 큐를 통해 처리 (RT-safe)
        template<typename... Args>
        NamedLogRtStream &format(fmt::format_string<Args...> fmtStr, Args&&... args) noexcept
        {
            if (!m_active)
            {
                return *this;
            }
            m_submitted = true;
            Instance().LogRtNamedFmt(m_logName, m_logLevel, fmtStr, std::forward<Args>(args)...);
            return *this;
        }

    private:
        LogLevel m_logLevel;
        bool     m_active;
        bool     m_submitted;
        size_t   m_pos;
        char     m_logName[NAME_BUF_LEN];
        char     m_buf[BUF_LEN];
        bool     m_hexMode{false};
        int      m_width{0};
        char     m_fillChar{' '};

    private:
        void Append(const char *src, size_t len) noexcept;

        template<typename T>
        bool FormatElement(T value) noexcept
        {
            if (m_pos + 10 >= BUF_LEN)
            {
                return false;
            }
            int written = 0;
            if constexpr (std::is_pointer_v<T>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%p", static_cast<const void*>(value));
            else if constexpr (std::is_enum_v<T>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%lld",
                              static_cast<long long>(static_cast<std::underlying_type_t<T>>(value)));
            else if constexpr (std::is_same_v<T, bool>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%s", value ? "true" : "false");
            else if constexpr (std::is_floating_point_v<T>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%.6f", static_cast<double>(value));
            else if constexpr (std::is_signed_v<T>)
                written = FormatIntState(static_cast<long long>(value), true);
            else
                written = FormatIntState(static_cast<long long>(static_cast<unsigned long long>(value)), false);

            if (written < 0 || written >= static_cast<int>(BUF_LEN - m_pos))
            {
                return false;
            }

            m_pos += written;
            if (m_pos < BUF_LEN)
            {
                m_buf[m_pos] = '\0';
            }
            return true;
        }

        void AddTruncation() noexcept;
        bool AddSeparator() noexcept;
        void CloseArray() noexcept;
        int FormatIntState(long long val, bool is_signed) noexcept;
    };

    // ── LogRtContStream ──────────────────────────────────────────────────────
    // Stream for LOG_CONT(level). Sends raw message (no indent) to drain thread.
    // The drain thread buffers and adds the 21-space indent on complete lines.
    // No automatic newline: multiple LOG_CONT calls concatenate on the same line;
    // only an explicit '\n' in the message causes a line break.
    //
    // Usage: LOG_CONT(info) << "part1 ";
    //        LOG_CONT(info) << "part2\n";
    // Output: "                     part1 part2\n"
    class [[nodiscard]] LogRtContStream
    {
    public:
        static constexpr size_t BUF_LEN = QueueType::MsgLen();

        explicit LogRtContStream(LogLevel lvl) noexcept;

        LogRtContStream(const LogRtContStream &)            = delete;
        LogRtContStream &operator=(const LogRtContStream &) = delete;
        LogRtContStream(LogRtContStream &&)                 = delete;
        LogRtContStream &operator=(LogRtContStream &&)      = delete;

        ~LogRtContStream() noexcept;

        template<typename T, typename = std::enable_if_t<
            std::is_arithmetic<T>::value ||
            std::is_pointer<T>::value ||
            std::is_enum<T>::value>>
        LogRtContStream &operator<<(T value) noexcept;

        template<typename T>
        LogRtContStream &operator<<(const std::vector<T> &value) noexcept;

        LogRtContStream &operator<<(const char *str) noexcept;
        LogRtContStream &operator<<(const std::string &str) noexcept;
        LogRtContStream &operator<<(char c) noexcept;
        LogRtContStream &operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept;
        LogRtContStream &operator<<(decltype(std::setw(0)) w) noexcept;
        LogRtContStream &operator<<(decltype(std::setfill(' ')) f) noexcept;

        LogRtContStream &printf(const char *fmt, ...) noexcept __attribute__((format(printf, 2, 3)));

    private:
        LogLevel m_logLevel;
        bool     m_active;
        size_t   m_pos;
        char     m_buf[BUF_LEN];
        bool     m_hexMode{false};
        int      m_width{0};
        char     m_fillChar{' '};

    private:
        void Append(const char *src, size_t len) noexcept;

        template<typename T>
        bool FormatElement(T value) noexcept
        {
            if (m_pos + 10 >= BUF_LEN) return false;
            int written = 0;
            if constexpr (std::is_pointer_v<T>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%p", static_cast<const void*>(value));
            else if constexpr (std::is_enum_v<T>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%lld",
                              static_cast<long long>(static_cast<std::underlying_type_t<T>>(value)));
            else if constexpr (std::is_same_v<T, bool>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%s", value ? "true" : "false");
            else if constexpr (std::is_floating_point_v<T>)
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%.6f", static_cast<double>(value));
            else if constexpr (std::is_signed_v<T>)
                written = FormatIntState(static_cast<long long>(value), true);
            else
                written = FormatIntState(static_cast<long long>(static_cast<unsigned long long>(value)), false);
            if (written < 0 || written >= static_cast<int>(BUF_LEN - m_pos)) return false;
            m_pos += written;
            if (m_pos < BUF_LEN) m_buf[m_pos] = '\0';
            return true;
        }

        void AddTruncation() noexcept;
        bool AddSeparator() noexcept;
        void CloseArray() noexcept;
        int FormatIntState(long long val, bool is_signed) noexcept;
    };

private:
    std::shared_ptr<spdlog::logger>  m_logger;
    std::shared_ptr<Log::RtTui>      m_tui;   // TUI instance (if enabled)
    int64_t                          m_tuiLastRender_ns{0};  // last TUI render timestamp (25 Hz rate-limiter)
    int64_t                          m_lastFlush_ns{0};        // last spdlog flush timestamp (100 ms rate-limiter)
    QueueType                        m_queue;
    TimeBase                         m_timebase;
    std::atomic<bool>                m_initialized;
    std::atomic<int>                 m_level;
    std::atomic<uint64_t>            m_dropCount;
    std::atomic<uint64_t>            m_syncSeq;  // Sync() 요청 시퀀스: 호출 시 증가
    std::atomic<uint64_t>            m_syncAck;  // drain 스레드가 DrainAll() 완료 후 갱신
    std::atomic<bool>                m_logThreadRun;
    struct ThreadInfo_Impl;
    std::unique_ptr<ThreadInfo_Impl> m_logThreadInfo;

    // LOG_CONT buffering — drain thread only, no locking needed
    static constexpr char            CONT_ENTRY_MARKER = '\x01';
    static constexpr size_t          CONT_BUF_SIZE     = RtLogConstant::QUEUE_MSGLEN * 4;
    char                             m_contBuf[CONT_BUF_SIZE];
    size_t                           m_contBufLen{0};
    spdlog::level::level_enum        m_contLevel{spdlog::level::info};
    std::string                      m_patternStr;  // current spdlog pattern, restored after %v switch

private:
    RtLog() noexcept;
    ~RtLog();

    // prevent copy and move
    RtLog(const RtLog &)            = delete;
    RtLog &operator=(const RtLog &) = delete;
    RtLog(RtLog &&)                 = delete;
    RtLog &operator=(RtLog &&)      = delete;

    void Poll() noexcept;
    void FlushEntry(const Entry &entry) noexcept;
    void LogRtCont(LogLevel lvl, const char *msg, size_t msgLen) noexcept;

    // Flush complete lines (ending with '\n') from m_contBuf.
    // If force==true, flush any remaining partial line too.
    // Called from drain thread only — no locking needed.
    void FlushContLines(bool force) noexcept;

    void Enqueue(const Entry &entry) noexcept;
    bool IsActiveLevel(LogLevel lvl) const noexcept;

    inline int64_t MonoNow_ns() const noexcept
    {
        struct timespec ts;
        // CLOCK_MONOTONIC is intercepted by Xenomai POSIX skin and runs in primary
        // mode. CLOCK_MONOTONIC_RAW is a Linux-only clock that falls through to the
        // Linux kernel, triggering a primary -> secondary mode switch on every call.
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + static_cast<int64_t>(ts.tv_nsec));
    }

    std::string AnnotateFilenameDatetime(const std::string &fileBasename);
    std::tuple<std::string, std::string> SplitByDirectory(const std::string &fname);
    bool EnsureDirectoryExistes(const std::string &dname, std::error_code &ec);
};  // class RtLog

};  // namespace Log

};  // namespace dt

// LOG_RT(level): returns a LogRtStream temporary — identical usage to dtLog's LOG(level).
//   LOG_RT(info) << "msg: " << val;
//   LOG(info).format("x={:.3f} idx={}", x, idx);
//   LOG(warn) << "q=" << q;   // dt::Math::Vector / Eigen: use operator<<, not format()
#define LOG(level) \
    dt::Log::RtLog::LogRtStream(dt::Log::LogLevel::level)

// LOG_RT_RAW: immediate write to STDERR, bypassing drain thread and log queue.
//
// Use when the drain thread may be unavailable or is itself the error source:
//   - Early init / late shutdown sequences
//   - Logger module internal errors
//   - Queue-full or drop-detected conditions
//   - Signal handlers (sigdebug_handler, etc.)
//
// Output goes to STDERR (not stdout), so it appears even when TUI owns stdout.
// In Xenomai: triggers secondary-mode switch (write() syscall) — acceptable for error paths.
//
// Example:
//   LOG_RT_RAW(err,      "queue full: dropped %llu messages", drop_cnt);
//   LOG_RT_RAW(critical, "logger init failed, errno=%d", errno);
#define LOG_RT_RAW(level, fmt, ...) \
    dt::Log::RtLog::LogRaw(dt::Log::LogLevel::level, fmt, ##__VA_ARGS__)

// LOG_U: A stream macro that logs to a named logger created with Create().
// As with LOG(), it is processed by the drain thread through the MPSC queue, so it can be used in RT tasks.
// Usage: LOG_U(logger_name, info) << "msg " << value;
//        LOG_U(logger_name, warn).printf("x=%.3f", x);
//        LOG_U(logger_name, debug).format("x={:.3f} idx={}", x, idx);
#define LOG_U(log_name, level) \
    dt::Log::RtLog::NamedLogRtStream(#log_name, dt::Log::LogLevel::level)

// LOG_CONT(level): continuation log — no prefix, 21-space indent, no automatic newline.
// Multiple calls concatenate on the same line; an explicit '\n' breaks the line.
// Example:
//   LOG_CONT(info) << "part1 ";
//   LOG_CONT(info) << "part2\n";
//   // Output: "                     part1 part2\n"
#define LOG_CONT(level) \
    dt::Log::RtLog::LogRtContStream(dt::Log::LogLevel::level)

// ═══════════════════════════════════════════════════════════════════════════
// TUI Area 1 macros — RT-safe, no-op when TUI is disabled
// All macros take layoutIdx as the first argument (0-based, key '1'~'9').
// ═══════════════════════════════════════════════════════════════════════════

// Group setup — call once at startup
// Example: TUI_SET_GROUP(0, grp, "Joints", "[status]", "desPos", "actPos")
#define TUI_SET_GROUP(layout, grp, label, ...) \
    dt::Log::RtLog::Instance().TuiSetGroup(layout, grp, label, ##__VA_ARGS__)

// Headerless group — data rows only, no label/column-header/underline rows
// Example: TUI_SET_GROUP_NO_HDR(0, grp)
#define TUI_SET_GROUP_NO_HDR(layout, grp) \
    dt::Log::RtLog::Instance().TuiSetGroupNoHeader(layout, grp)

// Layout name — shown in the bottom status bar
// Example: TUI_SET_LAYOUT_NAME(0, "Arm Control")
#define TUI_SET_LAYOUT_NAME(layout, name) \
    dt::Log::RtLog::Instance().TuiSetLayoutName(layout, name)

// Format-based row update — same format applied to every column value
// Example: TUI_SET_ROW(0, grp, row, "label", "%.2f", v1, v2, v3)
#define TUI_SET_ROW(layout, grp, row, label, fmt, ...) \
    dt::Log::RtLog::Instance().TuiSetRow(layout, grp, row, label, fmt, ##__VA_ARGS__)

// Per-column row update — TUI_COL("fmt", value) per column, types can differ
// Example: TUI_SET_ROW_COLS(0, grp, row, "R1",
//              TUI_COL("0x%04X", status), TUI_COL("%+8.1f", pos), TUI_COL("%+8d", tpu))
#define TUI_COL(fmt, val)   dt::Log::RtTui::TuiCol(fmt, val)

#define TUI_SET_ROW_COLS(layout, grp, row, label, ...) \
    dt::Log::RtLog::Instance().TuiSetRowCols(layout, grp, row, label, ##__VA_ARGS__)

// Full-width text row — ignores column layout, max 200 chars
// Example: TUI_SET_TEXT_ROW(0, grp, row, "label", text)
#define TUI_SET_TEXT_ROW(layout, grp, row, label, text) \
    dt::Log::RtLog::Instance().TuiSetTextRow(layout, grp, row, label, text)

// printf-style text row
// Example: TUI_SET_TEXT_ROW_FMT(0, grp, row, "Right", "Pos:%+8.3f,%+8.3f", px, py)
#define TUI_SET_TEXT_ROW_FMT(layout, grp, row, label, fmt, ...) \
    dt::Log::RtLog::Instance().TuiSetTextRowFmt(layout, grp, row, label, fmt, ##__VA_ARGS__)

// Keyboard input from TUI (non-blocking, returns 0 if none)
#define TUI_GET_PENDING_KEY()   dt::Log::RtLog::Instance().GetPendingKey()

// ═══════════════════════════════════════════════════════════════════════════
// TuiSinkT Implementation
// ═══════════════════════════════════════════════════════════════════════════
namespace dt {

namespace Log {

template<typename Mutex>
void TuiSinkT<Mutex>::sink_it_(const spdlog::details::log_msg &msg)
{
    if (!m_tui)
    {
        return;
    }

    msg.color_range_start = 0;
    msg.color_range_end   = 0;

    spdlog::memory_buf_t buf;
    Base::formatter_->format(msg, buf);

    size_t sz = buf.size();
    if (sz > 0 && buf[sz - 1] == '\n')
    {
        sz--;
    }

    const char *color = SinkColorFor(msg.level);
    if (*color && msg.color_range_end > msg.color_range_start && msg.color_range_end <= sz)
    {
        // Color applied to prefix ([L][timestamp]) only; message body uses default color.
        // Assembled in a stack buffer (no heap allocation) then pushed to the TUI queue.
        static constexpr size_t TMP_LEN = Log::RtTui::QUEUE_MSG_LEN;
        char tmp[TMP_LEN];
        size_t pos = 0;

        auto safe_copy = [&](const char *src, size_t len)
        {
            size_t n = std::min(len, TMP_LEN - 1 - pos);
            if (n)
            {
                std::memcpy(tmp + pos, src, n); pos += n;
            }
        };

        safe_copy(buf.data(), msg.color_range_start);                              // before color range
        safe_copy(color, std::strlen(color));                                      // color code
        safe_copy(buf.data() + msg.color_range_start, msg.color_range_end - msg.color_range_start); // colored prefix
        safe_copy("\033[m", 3);                                                    // reset
        safe_copy(buf.data() + msg.color_range_end, sz - msg.color_range_end);     // message body

        tmp[pos] = '\0';
        m_tui->Log(msg.level, "%s", tmp);
    }
    else
    {
        m_tui->Log(msg.level, "%.*s", (int)sz, buf.data());
    }
}

template<typename T, typename>
RtLog::LogRtStream &RtLog::LogRtStream::operator<<(T value) noexcept
{
    if (!m_active)
    {
        return *this;
    }

    if (!FormatElement(value))
    {
        AddTruncation();
    }

    return *this;
}

template <typename T>
RtLog::LogRtStream &RtLog::LogRtStream::operator<<(const std::vector<T> &value) noexcept
{
    if (!m_active)
    {
        return *this;
    }

    // Open array bracket
    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }
    m_buf[m_pos++] = '[';

    const size_t count = value.size();
    for (size_t i = 0; i < count; i++)
    {
        // Format current element
        if (!FormatElement(value[i]))
        {
            AddTruncation();
            break;
        }

        // Add separator if not the last element
        if (i != count - 1)
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

template<typename T, typename>
RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(T value) noexcept
{
    if (!m_active)
    {
        return *this;
    }

    if (!FormatElement(value))
    {
        AddTruncation();
    }
    return *this;
}

template<typename T>
RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(const std::vector<T> &value) noexcept
{
    if (!m_active)
    {
        return *this;
    }

    if (m_pos + 1 >= BUF_LEN)
    {
        return *this;
    }
    m_buf[m_pos++] = '[';

    const size_t count = value.size();
    for (size_t i = 0; i < count; i++)
    {
        if (!FormatElement(value[i]))
        {
            AddTruncation();
            break;
        }
        if (i != count - 1)
        {
            if (!AddSeparator())
            {
                break;
            }
        }
    }

    CloseArray();
    return *this;
}

// ── LogRtContStream operator<< implementations ────────────────────────────────

template<typename T, typename>
RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(T value) noexcept
{
    if (!m_active) return *this;
    if (!FormatElement(value)) AddTruncation();
    return *this;
}

template<typename T>
RtLog::LogRtContStream &RtLog::LogRtContStream::operator<<(const std::vector<T> &value) noexcept
{
    if (!m_active) return *this;
    if (m_pos + 1 >= BUF_LEN) return *this;
    m_buf[m_pos++] = '[';
    const size_t count = value.size();
    for (size_t i = 0; i < count; i++)
    {
        if (!FormatElement(value[i])) { AddTruncation(); break; }
        if (i != count - 1 && !AddSeparator()) break;
    }
    CloseArray();
    return *this;
}

inline void Initialize(
    const std::string &logName,
    const std::string &fileBasename = "",
    bool enableTui = false,
    int threadCpuId = RtLogConstant::THREAD_CPU_ID,
    size_t maxFiles = RtLogConstant::DEFAULT_MAX_FILES,
    size_t maxFileSize = RtLogConstant::DEFAULT_MAX_SIZE,
    int threadPriority = RtLogConstant::THREAD_PRIORITY,
    size_t threadStack = RtLogConstant::THREAD_STACK_SIZE,
    bool annotDatetime = true,
    bool truncate = false)
{
    RtLog::Initialize(logName, fileBasename, enableTui, threadCpuId, maxFiles, maxFileSize, threadPriority, threadStack, annotDatetime, truncate);
}

inline void Create(
    const std::string &logName,
    const std::string &fileBasename,
    bool annotDatetime = true,
    bool truncate = false,
    size_t maxFiles = RtLogConstant::DEFAULT_MAX_FILES,
    size_t maxFileSize = RtLogConstant::DEFAULT_MAX_SIZE)
{
    RtLog::Create(logName, fileBasename, annotDatetime, truncate, maxFiles, maxFileSize);
}

inline void Terminate()
{
    RtLog::Terminate();
}

inline void FlushOn(LogLevel lvl)
{
    RtLog::FlushOn(lvl);
}

inline void FlushOn(const std::string &logger_name, LogLevel lvl)
{
    RtLog::FlushOn(logger_name, lvl);
}

inline void SetLogLevel(LogLevel lvl)
{
    RtLog::SetLogLevel(lvl);
}

inline void SetLogLevel(const std::string &logger, LogLevel lvl)
{
    RtLog::SetLogLevel(logger, lvl);
}

inline void SetLogPattern(LogPattern pattern = LogPatternFlag::type|LogPatternFlag::time, const std::string &delimiter = "|")
{
    RtLog::SetLogPattern(pattern, delimiter);
}

inline void SetLogPattern(const std::string &logger, LogPattern pattern, const std::string &delimiter = "|")
{
    RtLog::SetLogPattern(logger, pattern, delimiter);
}

inline void SetLogPattern(const std::string &raw_pattern)
{
    RtLog::SetLogPattern(raw_pattern);
}

inline void SetLogPattern(const std::string &logger, const std::string &raw_pattern)
{
    RtLog::SetLogPattern(logger, raw_pattern);
}

}   // namespace Log

}   // namespace dt
#endif  // _DT_RTLOG_H_
