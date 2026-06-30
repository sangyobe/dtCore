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

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/details/os.h>
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
#include <thread>
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

namespace dt 
{
namespace RtLogConstant
{
    inline constexpr size_t DEFAULT_MAX_SIZE  = 10 * 1024 * 1024;  // 10MB
    inline constexpr size_t DEFAULT_MAX_FILES = 5;
    inline constexpr size_t INTERNAL_BUF_SIZE = 65536;  // 64 KB internal buffer
    inline constexpr size_t QUEUE_CAPACITY    = 1024;
    inline constexpr size_t QUEUE_MSGLEN      = 1024;
    // Maximum delay between log output bursts (nanoseconds)
    inline constexpr long POLL_INTERVAL_NS    = 1'000'000L; // 1 ms
    // Thread info
    inline constexpr size_t THREAD_STACK_SIZE = 1024 * 1024; // 1MB
    inline constexpr int THREAD_CPU_ID        = 2;  // default CPU core(#2)
    inline constexpr int THREAD_PRIORITY      = 0;  // nonRt
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
    explicit TuiSinkT(std::shared_ptr<Utils::RtTui> tui) : m_tui(tui) {}

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override;
    void flush_() override {}

private:
    std::shared_ptr<Utils::RtTui> m_tui;
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
    using LogLevel = spdlog::level::level_enum;
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
        bool truncate = false) 
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
            m_instance.m_tui = std::make_shared<Utils::RtTui>();
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
        m_instance.m_timebase = TimeBase::Capture();
        m_instance.m_initialized.store(true, std::memory_order_release);

        // Create log thread
        m_instance.m_logThreadInfo.name = "RtLogThread";
        m_instance.m_logThreadInfo.stackSz = threadStack;
        m_instance.m_logThreadInfo.cpuIdx = threadCpuId;
        m_instance.m_logThreadInfo.priority = threadPriority;
        m_instance.m_logThreadInfo.procFunc = PollLogQueue;
        m_instance.m_logThreadInfo.procFuncArg = nullptr;
        m_instance.m_logThreadInfo.run.store(true, std::memory_order_release);

        pthread_attr_t taskAttr;
        pthread_attr_init(&taskAttr);
        
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);               // removes all CPUs from cpuset
        CPU_SET(threadCpuId, &cpuset); // add CPU idx to the cpuset
        pthread_attr_setinheritsched(&taskAttr, PTHREAD_EXPLICIT_SCHED);
        if (m_instance.m_logThreadInfo.priority > 0) 
        {
            struct sched_param taskParam = {.sched_priority = threadPriority};
            pthread_attr_setschedpolicy(&taskAttr, SCHED_FIFO);
            pthread_attr_setschedparam(&taskAttr, &taskParam);
        }
        else 
        {
            pthread_attr_setschedpolicy(&taskAttr, SCHED_OTHER);
        }
        pthread_attr_setaffinity_np(&taskAttr, sizeof(cpuset), &cpuset);
        pthread_attr_setdetachstate(&taskAttr, PTHREAD_CREATE_JOINABLE);
        pthread_attr_setstacksize(&taskAttr, threadStack);

        int ret = pthread_create(&m_instance.m_logThreadInfo.id, 
                                 &taskAttr, 
                                 m_instance.m_logThreadInfo.procFunc, 
                                 m_instance.m_logThreadInfo.procFuncArg);
        if (ret < 0) 
        {
            m_instance.LogRaw(LogLevel::err, "[RTLOG] Create logger thread failed!!!!");
        }

        pthread_attr_destroy(&taskAttr);
    }

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
        size_t maxFileSize = RtLogConstant::DEFAULT_MAX_SIZE)
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

    /**
     * Terminate logging system
     */
    static void Terminate()
    {
        auto &m_instance = Instance();

        // Report any messages that were dropped during operation
        uint64_t drops = m_instance.DropCount();
        if (drops > 0)
        {
            m_instance.m_logger->log(spdlog::level::warn, "CloseLogger: RT log queue dropped %llu messages during operation", (unsigned long long)drops);
        }

        // thread join
        m_instance.m_logThreadInfo.run.store(false, std::memory_order_release);
        pthread_join(m_instance.m_logThreadInfo.id, nullptr);
        m_instance.m_logThreadInfo.id = {};
        
        // last flush
        m_instance.DrainAll();

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

    // dummy function for migration
    static void FlushOn(LogLevel lvl)
    {
        Instance().LogRt(LogLevel::debug, "[RtLog] No need: FlushOn()");
    }

    // dummy function for migration
    static void FlushOn(const std::string &logger_name, LogLevel lvl)
    {
        Instance().LogRt(LogLevel::debug, "[RtLog] No need: FlushOn()");
    }

    /**
     * Set log level of default logger
     * @param lvl log level
     */
    static void SetLogLevel(LogLevel lvl) 
    {
        Instance().SetLevel(lvl);
    }

    /**
     * Set log level of custom logger
     * @param logger_name name of custom logger
     * @param lvl log level
     */
    static void SetLogLevel(const std::string &logger_name, LogLevel lvl)
    {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
        if (logger) 
        {
            logger->set_level(lvl);
        }
    }

    /**
     * Default logger 로그 패턴 설정 (flag 조합 방식).
     * @param pattern LogPatternFlag 조합.
     * @param delimiter 각 항목 사이의 구분자 문자열 (e.g. "|", " ").
     *
     * 주의: spdlog 원시 패턴 문자열을 직접 사용하려면
     *       SetLogPattern(const std::string &raw_pattern) 오버로드를 사용하세요.
     */
    static void SetLogPattern(LogPattern pattern = LogPatternFlag::type|LogPatternFlag::time, const std::string &delimiter = "|")
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
        Sync();  // 이미 큐에 쌓인 메시지가 모두 이전 패턴으로 출력된 뒤 패턴을 변경
        auto &inst = Instance();
        if (inst.m_logger)
        {
            inst.m_logger->set_pattern(pattern_str);
        }
    }

    static void SetLogPattern(const std::string &logger_name, LogPattern pattern = LogPatternFlag::type|LogPatternFlag::time, const std::string &delimiter = "|")
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

    static void SetLogPattern(const std::string &logger_name, const std::string &raw_pattern)
    {
        Sync();  // drain queued messages and flush all sinks before changing the pattern
        std::shared_ptr<spdlog::logger> logger = spdlog::get(logger_name);
        if (logger)
        {
            logger->flush();
            logger->set_pattern(raw_pattern);
        }
    }

    static void SetLogPattern(const std::string &raw_pattern)
    {
        Sync();  // 이미 큐에 쌓인 메시지가 모두 이전 패턴으로 출력된 뒤 패턴을 변경
        auto &inst = Instance();
        if (inst.m_logger) inst.m_logger->set_pattern(raw_pattern);
    }

    // Returns the TUI instance (nullptr when TUI is disabled)
    std::shared_ptr<Utils::RtTui> GetTui() const noexcept 
    {
        return m_tui;
    }

    // Get the latest pending key from TUI (non-blocking, returns 0 if no key)
    char GetPendingKey() const noexcept 
    {
        return m_tui ? m_tui->PopPendingKey() : 0;
    }

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

    void TuiSetGroupNoHeader(int layoutIdx, int groupIdx) noexcept 
    {
        if (m_tui)
        {
            m_tui->SetGroupNoHeader(layoutIdx, groupIdx);
        }
    }

    void TuiSetTextRow(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *text) noexcept 
    {
        if (m_tui) 
        {
            m_tui->SetTextRow(layoutIdx, groupIdx, rowIdx, label, text);
        }
    }

    void TuiSetTextRowFmt(int layoutIdx, int groupIdx, int rowIdx, const char *label, const char *fmt, ...) noexcept
        __attribute__((format(printf, 6, 7))) 
    {
        if (!m_tui)
        {
            return;
        }

        char buf[Utils::RtTui::TUI_TEXT_ROW_LEN];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        m_tui->SetTextRow(layoutIdx, groupIdx, rowIdx, label, buf);
    }

    void TuiSetLayoutName(int layoutIdx, const char *name) noexcept 
    {
        if (m_tui) 
        {
            m_tui->SetLayoutName(layoutIdx, name);
        }
    }

    bool IsInitialized() const noexcept 
    {
        return m_initialized.load(std::memory_order_acquire);
    }

    // Called repeatedly by the log thread in a loop.
    // Sleeps POLL_INTERVAL_NS then drains all queued entries.
    // No syscall occurs in the RT producer path.
    static void *PollLogQueue(void *pArg) noexcept 
    {
        auto &m_instance = Instance();
        
        while (m_instance.m_logThreadInfo.run.load(std::memory_order_acquire))
        {
            Instance().Poll();
        }
        
        return nullptr;
    }

    void RefreshTimebase() noexcept 
    {
        m_timebase = TimeBase::Capture();
    }

    // Flush all remaining entries from the RT queue.
    // IMPORTANT: must only be called from a single consumer thread at a time.
    // LogQueue is MPSC — concurrent try_pop() from two threads is undefined behaviour.
    // Callers must ensure the drain thread has stopped before calling this (e.g. in Terminate()).
    size_t DrainAll() noexcept 
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

    // drain 스레드가 현재 큐에 있는 모든 메시지를 처리할 때까지 대기.
    //
    // SetLogPattern() / SetLogLevel() 등 포맷에 영향을 주는 변경 전에 호출하면
    // "이미 쌓인 메시지가 새 패턴으로 출력되는" 문제를 방지할 수 있음.
    //
    // 동작 원리:
    //   1. m_syncSeq를 1 증가시켜 drain 스레드에 동기화 요청을 등록.
    //   2. drain 스레드는 DrainAll() 완료 직후 m_syncAck = m_syncSeq 로 응답.
    //   3. m_syncAck >= target 이 될 때까지 500μs 간격으로 대기.
    //   4. 대기 후 sink 내부 버퍼를 flush() 하여 이전 메시지가 출력 장치에 반영되도록 함.
    //
    // 비RT 컨텍스트에서만 사용 가능 (clock_nanosleep 사용).
    static void Sync() noexcept
    {
        auto &inst = Instance();
        if (!inst.m_initialized.load(std::memory_order_acquire))
        {
            return;
        }

        uint64_t target = inst.m_syncSeq.fetch_add(1, std::memory_order_acq_rel) + 1;

        while (inst.m_syncAck.load(std::memory_order_acquire) < target)
        {
            struct timespec ts{0, 500000L};  // 500 μs
            clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
        }

        // Flush all registered loggers (default + every named logger created via Create()).
        // Named loggers use their own sinks whose internal buffers are not flushed elsewhere.
        spdlog::apply_all([](std::shared_ptr<spdlog::logger> _logger) { _logger->flush(); });
    }

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
    static void LogRaw(LogLevel lvl, const char *fmt, ...) noexcept 
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

    void LogRtV(LogLevel lvl, const char* format, va_list args) noexcept 
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

    // Named logger 경로 — LOG_U() 소멸자에서 호출. RT 큐를 통해 drain 스레드가 처리.
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

    void LogRtNamedV(const char *loggerName, LogLevel lvl, const char *format, va_list args) noexcept
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

    void SetLevel(LogLevel lvl) noexcept
    {
        m_level.store(static_cast<int>(lvl), std::memory_order_relaxed);
        // Synchronize with spdlog logger level
        if (m_logger) 
        {
            m_logger->set_level(lvl);
        }
    }

    LogLevel GetLevel() const noexcept 
    {
        return static_cast<LogLevel>(m_level.load(std::memory_order_relaxed));
    }

    uint64_t DropCount() const noexcept 
    {
        return m_dropCount.load(std::memory_order_relaxed);
    }

    // Get approximate number of messages currently in queue
    size_t QueueSize() const noexcept 
    {
        return m_queue.ApproxSize();
    }

    // Get queue utilization percentage (0-100)
    size_t QueueUtilization() const noexcept 
    {
        size_t size = m_queue.ApproxSize();
        return ((size * 100) / QueueType::Capacity());
    }

    // Get queue statistics
    struct QueueStats 
    {
        size_t current_size;      // Current number of messages in queue
        size_t capacity;          // Maximum queue capacity
        size_t utilization_pct;   // Utilization percentage (0-100)
        uint64_t total_drops;     // Total number of dropped messages
    };

    QueueStats GetQueueStats() const noexcept 
    {
        size_t size = m_queue.ApproxSize();
        return QueueStats{
            .current_size = size,
            .capacity = QueueType::Capacity(),
            .utilization_pct = (size * 100) / QueueType::Capacity(),
            .total_drops = m_dropCount.load(std::memory_order_relaxed)
        };
    }

    class [[nodiscard]] LogRtStream 
    {
    public:
        static constexpr size_t BUF_LEN = QueueType::MsgLen();
                
        LogRtStream(LogLevel lvl) noexcept
            : m_logLevel(lvl),
              m_active(Instance().IsActiveLevel(lvl)),
              m_pos(0)
        {
            m_buf[0] = '\0';
        }

        // prevent copy and move
        LogRtStream(const LogRtStream &)            = delete;
        LogRtStream &operator=(const LogRtStream &) = delete;
        LogRtStream(LogRtStream &&)                 = delete;
        LogRtStream &operator=(LogRtStream &&)      = delete;

        ~LogRtStream() noexcept {
            if (m_active && !m_submitted && m_pos > 0)
            {
                Instance().LogRt(m_logLevel, "%.*s", (int)m_pos, m_buf);
            }
        }

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
        void Append(const char *src, size_t len) noexcept 
        {
            size_t avail = BUF_LEN - 1 - m_pos;
            size_t copy  = (len < avail) ? len : avail;
            memcpy(m_buf + m_pos, src, copy);
            m_pos        += copy;
            m_buf[m_pos]  = '\0';
        }

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
        void AddTruncation() noexcept 
        {
            if (m_pos + 3 < BUF_LEN) 
            {
                m_buf[m_pos++] = '.';
                m_buf[m_pos++] = '.';
                m_buf[m_pos++] = '.';
                m_buf[m_pos] = '\0';
            }
        }

        // Helper to add separator ", "
        bool AddSeparator() noexcept 
        {
            if (m_pos + 2 < BUF_LEN) 
            {
                m_buf[m_pos++] = ',';
                m_buf[m_pos++] = ' ';
                return true;
            }
            return false;
        }

        // Helper to close array with ']'
        void CloseArray() noexcept
        {
            if (m_pos + 1 < BUF_LEN)
            {
                m_buf[m_pos++] = ']';
                m_buf[m_pos] = '\0';
            }
        }

        // Format integer with current hex/width/fill state (via m_hexMode, m_width, m_fillChar).
        // m_width is one-shot (reset after use). m_hexMode and m_fillChar are sticky.
        // Supports fill chars '0' and ' ' only (snprintf limitation).
        int FormatIntState(long long val, bool is_signed) noexcept
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
    };

    // NamedLogRtStream: Create()로 생성한 named logger에 기록하는 RT-safe 스트림 인터페이스.
    // LOG()와 동일하게 MPSC 큐를 통해 drain 스레드에서 처리하므로 RT 태스크에서 사용 가능.
    // spdlog::get()은 drain 스레드(비RT)의 FlushEntry()에서만 호출됨.
    class [[nodiscard]] NamedLogRtStream
    {
    public:
        static constexpr size_t BUF_LEN      = QueueType::MsgLen();
        static constexpr size_t NAME_BUF_LEN = 64;

        explicit NamedLogRtStream(const char *logName, LogLevel lvl) noexcept
            : m_logLevel(lvl),
              m_active(Instance().IsInitialized() && Instance().IsActiveLevel(lvl)),
              m_submitted(false),
              m_pos(0)
        {
            strncpy(m_logName, logName, NAME_BUF_LEN - 1);
            m_logName[NAME_BUF_LEN - 1] = '\0';
            m_buf[0] = '\0';
        }

        NamedLogRtStream(const NamedLogRtStream &)            = delete;
        NamedLogRtStream &operator=(const NamedLogRtStream &) = delete;
        NamedLogRtStream(NamedLogRtStream &&)                 = delete;
        NamedLogRtStream &operator=(NamedLogRtStream &&)      = delete;

        ~NamedLogRtStream() noexcept
        {
            if (m_active && !m_submitted && m_pos > 0)
            {
                Instance().LogRtNamed(m_logName, m_logLevel, "%.*s", (int)m_pos, m_buf);
            }
        }

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
        void Append(const char *src, size_t len) noexcept
        {
            size_t avail = BUF_LEN - 1 - m_pos;
            size_t copy  = (len < avail) ? len : avail;
            memcpy(m_buf + m_pos, src, copy);
            m_pos       += copy;
            m_buf[m_pos] = '\0';
        }

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

        void AddTruncation() noexcept
        {
            if (m_pos + 3 < BUF_LEN)
            {
                m_buf[m_pos++] = '.';
                m_buf[m_pos++] = '.';
                m_buf[m_pos++] = '.';
                m_buf[m_pos]   = '\0';
            }
        }

        bool AddSeparator() noexcept
        {
            if (m_pos + 2 < BUF_LEN)
            {
                m_buf[m_pos++] = ',';
                m_buf[m_pos++] = ' ';
                return true;
            }
            return false;
        }

        void CloseArray() noexcept
        {
            if (m_pos + 1 < BUF_LEN)
            {
                m_buf[m_pos++] = ']';
                m_buf[m_pos]   = '\0';
            }
        }

        int FormatIntState(long long val, bool is_signed) noexcept
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
    };

private:
    typedef struct _threadInfo
    {
        const char *name = nullptr;
        void *(*procFunc)(void *arg) = nullptr;
        void *procFuncArg = nullptr;
        int cpuIdx = 0;
        int priority = 0;
        size_t stackSz = 0;
        pthread_t id = 0;
        int listIdx = 0;
        std::atomic<bool> run;
    } ThreadInfo;

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::shared_ptr<Utils::RtTui> m_tui;   // TUI instance (if enabled)
    int64_t               m_tuiLastRender_ns{0};  // last TUI render timestamp (25 Hz rate-limiter)
    int64_t               m_lastFlush_ns{0};        // last spdlog flush timestamp (100 ms rate-limiter)
    QueueType             m_queue;
    TimeBase              m_timebase;
    std::atomic<bool>     m_initialized;
    std::atomic<int>      m_level;
    std::atomic<uint64_t> m_dropCount;
    std::atomic<uint64_t> m_syncSeq;  // Sync() 요청 시퀀스: 호출 시 증가
    std::atomic<uint64_t> m_syncAck;  // drain 스레드가 DrainAll() 완료 후 갱신
    ThreadInfo            m_logThreadInfo;

private:
    RtLog() noexcept
        : m_logger(nullptr),
          m_tui(nullptr),
          m_tuiLastRender_ns(0),
          m_lastFlush_ns(0),
          m_timebase{},
          m_initialized(false)
    {
        m_level.store(static_cast<int>(LogLevel::trace), std::memory_order_relaxed);
        m_dropCount.store(0, std::memory_order_relaxed);
        m_syncSeq.store(0, std::memory_order_relaxed);
        m_syncAck.store(0, std::memory_order_relaxed);
    }

    ~RtLog() = default;

    // prevent copy and move
    RtLog(const RtLog &)            = delete;
    RtLog &operator=(const RtLog &) = delete;
    RtLog(RtLog &&)                 = delete;
    RtLog &operator=(RtLog &&)      = delete;

    void Poll() noexcept 
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
        // apply_all covers both the default logger and every named logger created via Create().
        const int64_t now_ns = MonoNow_ns();
        if (now_ns - m_lastFlush_ns >= 100'000'000LL)
        {  // 100 ms
            spdlog::apply_all([](std::shared_ptr<spdlog::logger> l) { l->flush(); });
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

    void FlushEntry(const Entry &entry) noexcept
    {
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

    void Enqueue(const Entry &entry) noexcept 
    {
        // RT-safe: lock-free push only, no semaphore post, no syscall
        if (!m_queue.TryPush(entry)) 
        {
            // Only increment the drop counter; pushing into a full queue is pointless.
            // Monitor via drop_count() or display with TUI_SET_ROW_V.
            m_dropCount.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool IsActiveLevel(LogLevel lvl) const noexcept 
    {
        return (static_cast<int>(lvl) >= m_level.load(std::memory_order_relaxed));
    }

    inline int64_t MonoNow_ns() const noexcept 
    {
        struct timespec ts;
        // CLOCK_MONOTONIC is intercepted by Xenomai POSIX skin and runs in primary
        // mode. CLOCK_MONOTONIC_RAW is a Linux-only clock that falls through to the
        // Linux kernel, triggering a primary -> secondary mode switch on every call.
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return (static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + static_cast<int64_t>(ts.tv_nsec));
    }

    std::string AnnotateFilenameDatetime(const std::string &fileBasename) 
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

    std::tuple<std::string, std::string> SplitByDirectory(const std::string &fname) 
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

    bool EnsureDirectoryExistes(const std::string &dname, std::error_code &ec)
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
};  // class RtLog

using Log = RtLog;

};  // namespace dt

// LOG_RT(level): returns a LogRtStream temporary — identical usage to dtLog's LOG(level).
//   LOG_RT(info) << "msg: " << val;
//   LOG(info).format("x={:.3f} idx={}", x, idx);
//   LOG(warn) << "q=" << q;   // dt::Math::Vector / Eigen: use operator<<, not format()
#define LOG(level) \
    dt::RtLog::LogRtStream(dt::RtLog::LogLevel::level)

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
    dt::RtLog::LogRaw(dt::RtLog::LogLevel::level, fmt, ##__VA_ARGS__)

// LOG_U: Create()로 생성한 named logger에 기록하는 스트림 매크로.
// LOG()와 동일하게 MPSC 큐를 통해 drain 스레드에서 처리하므로 RT 태스크에서 사용 가능.
// Usage: LOG_U(logger_name, info) << "msg " << value;
//        LOG_U(logger_name, warn).printf("x=%.3f", x);
//        LOG_U(logger_name, debug).format("x={:.3f} idx={}", x, idx);
#define LOG_U(log_name, level) \
    dt::RtLog::NamedLogRtStream(#log_name, dt::RtLog::LogLevel::level)

// ═══════════════════════════════════════════════════════════════════════════
// TUI Area 1 macros — RT-safe, no-op when TUI is disabled
// All macros take layoutIdx as the first argument (0-based, key '1'~'9').
// ═══════════════════════════════════════════════════════════════════════════

// Group setup — call once at startup
// Example: TUI_SET_GROUP(0, grp, "Joints", "[status]", "desPos", "actPos")
#define TUI_SET_GROUP(layout, grp, label, ...) \
    dt::RtLog::Instance().TuiSetGroup(layout, grp, label, ##__VA_ARGS__)

// Headerless group — data rows only, no label/column-header/underline rows
// Example: TUI_SET_GROUP_NO_HDR(0, grp)
#define TUI_SET_GROUP_NO_HDR(layout, grp) \
    dt::RtLog::Instance().TuiSetGroupNoHeader(layout, grp)

// Layout name — shown in the bottom status bar
// Example: TUI_SET_LAYOUT_NAME(0, "Arm Control")
#define TUI_SET_LAYOUT_NAME(layout, name) \
    dt::RtLog::Instance().TuiSetLayoutName(layout, name)

// Format-based row update — same format applied to every column value
// Example: TUI_SET_ROW(0, grp, row, "label", "%.2f", v1, v2, v3)
#define TUI_SET_ROW(layout, grp, row, label, fmt, ...) \
    dt::RtLog::Instance().TuiSetRow(layout, grp, row, label, fmt, ##__VA_ARGS__)

// Per-column row update — TUI_COL("fmt", value) per column, types can differ
// Example: TUI_SET_ROW_COLS(0, grp, row, "R1",
//              TUI_COL("0x%04X", status), TUI_COL("%+8.1f", pos), TUI_COL("%+8d", tpu))
#define TUI_COL(fmt, val) \
    dt::Utils::RtTui::TuiCol(fmt, val)
#define TUI_SET_ROW_COLS(layout, grp, row, label, ...) \
    dt::RtLog::Instance().TuiSetRowCols(layout, grp, row, label, ##__VA_ARGS__)

// Full-width text row — ignores column layout, max 200 chars
// Example: TUI_SET_TEXT_ROW(0, grp, row, "label", text)
#define TUI_SET_TEXT_ROW(layout, grp, row, label, text) \
    dt::RtLog::Instance().TuiSetTextRow(layout, grp, row, label, text)

// printf-style text row
// Example: TUI_SET_TEXT_ROW_FMT(0, grp, row, "Right", "Pos:%+8.3f,%+8.3f", px, py)
#define TUI_SET_TEXT_ROW_FMT(layout, grp, row, label, fmt, ...) \
    dt::RtLog::Instance().TuiSetTextRowFmt(layout, grp, row, label, fmt, ##__VA_ARGS__)

// Keyboard input from TUI (non-blocking, returns 0 if none)
#define TUI_GET_PENDING_KEY() \
    dt::RtLog::Instance().GetPendingKey()

// ═══════════════════════════════════════════════════════════════════════════
// TuiSinkT Implementation
// ═══════════════════════════════════════════════════════════════════════════
namespace dt {

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
        static constexpr size_t TMP_LEN = Utils::RtTui::QUEUE_MSG_LEN;
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

template<typename T, typename = std::enable_if_t<
    std::is_arithmetic<T>::value ||
    std::is_pointer<T>::value ||
    std::is_enum<T>::value>>
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

inline RtLog::LogRtStream &RtLog::LogRtStream::operator<<(const char* str) noexcept 
{
    if (m_active && str)
    {
        Append(str, strnlen(str, BUF_LEN));
    }
    return *this;
}

inline RtLog::LogRtStream &RtLog::LogRtStream::operator<<(const std::string &str) noexcept 
{
    if (m_active)
    {
        Append(str.c_str(), str.size());
    }
    return *this;
}

inline RtLog::LogRtStream &RtLog::LogRtStream::operator<<(char c) noexcept 
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
inline RtLog::LogRtStream &RtLog::LogRtStream::printf(const char *fmt, ...) noexcept
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
inline RtLog::LogRtStream &RtLog::LogRtStream::operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept
{
    if (!m_active) return *this;
    if      (fn == std::hex) m_hexMode = true;
    else if (fn == std::dec) m_hexMode = false;
    return *this;
}

// std::setw(n) — GCC/libstdc++: returns std::_Setw{_M_n}.
inline RtLog::LogRtStream &RtLog::LogRtStream::operator<<(decltype(std::setw(0)) w) noexcept
{
    if (m_active) m_width = w._M_n;
    return *this;
}

// std::setfill(c) — GCC/libstdc++: returns std::_Setfill<char>{_M_c}.
// Only '0' and ' ' (default) take effect; other chars are accepted but fall back to space.
inline RtLog::LogRtStream &RtLog::LogRtStream::operator<<(decltype(std::setfill(' ')) f) noexcept
{
    if (m_active) m_fillChar = f._M_c;
    return *this;
}

// ═══════════════════════════════════════════════════════════════════════════
// NamedLogRtStream Implementations
// ═══════════════════════════════════════════════════════════════════════════

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

inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(const char *str) noexcept
{
    if (m_active && str)
    {
        Append(str, strnlen(str, BUF_LEN));
    }
    return *this;
}

inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(const std::string &str) noexcept
{
    if (m_active)
    {
        Append(str.c_str(), str.size());
    }
    return *this;
}

inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(char c) noexcept
{
    if (m_active && m_pos + 1 < BUF_LEN)
    {
        m_buf[m_pos++] = c;
        m_buf[m_pos]   = '\0';
    }
    return *this;
}

inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(std::ios_base &(*fn)(std::ios_base &)) noexcept
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

inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(decltype(std::setw(0)) w) noexcept
{
    if (m_active) 
    {
        m_width = w._M_n;
    }

    return *this;
}

inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::operator<<(decltype(std::setfill(' ')) f) noexcept
{
    if (m_active) 
    {
        m_fillChar = f._M_c;
    }

    return *this;
}

// printf-style: RT 큐를 통해 처리 (RT-safe)
inline RtLog::NamedLogRtStream &RtLog::NamedLogRtStream::printf(const char *fmt, ...) noexcept
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

}  // namespace dt
#endif  // _DT_RTLOG_H_
