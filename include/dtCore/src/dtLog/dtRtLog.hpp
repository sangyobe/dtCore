/*!
 \file      dtRtLog.hpp
 \brief     Logger for real-time system
 \author    myungjin.kim@hyundai.com
 \date      2026. 4. 24
 \version   0.0.2
 \copyright RoboticsLab ART All rights reserved.
*/
#pragma once
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

#include "dtLogQueue.hpp"
#include "dtRtTui.hpp"

// Forward declaration for optional Eigen support (include dtRtLogEigen.hpp for the implementation)
namespace Eigen {
    template<typename Derived>
    class MatrixBase;
}

// Forward declarations for optional dt::Math vector support (include dtRtLogDtMath.hpp for the implementation)
namespace dt { 
namespace Math {
    template<uint16_t t_row, typename t_type> class Vector;
    template<typename t_type, uint16_t t_row> class Vector3;
    template<typename t_type, uint16_t t_row> class Vector4;
    template<typename t_type, uint16_t t_row> class Vector6;
    template<typename t_type>                 class VectorX;
}
}

namespace dt {
namespace rtlog_constant {
    inline constexpr size_t DEFAULT_MAX_SIZE  = 10 * 1024 * 1024;  // 10MB
    inline constexpr size_t DEFAULT_MAX_FILES = 5;
    inline constexpr size_t OUT_BUF_SIZE      = 65536;  // 64 KB internal buffer
    inline constexpr size_t QUEUE_CAPACITY    = 1024;
    inline constexpr size_t QUEUE_MSG_LEN     = 1024;
    // Maximum delay between log output bursts (nanoseconds)
    inline constexpr long POLL_INTERVAL_NS    = 1'000'000L; // 1 ms
} // namespace rtlog_constant

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
inline const char* sink_color_for(spdlog::level::level_enum lvl) noexcept {
    switch (lvl) {
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
class ColorStdoutSinkT final : public spdlog::sinks::base_sink<Mutex> {
    using Base = spdlog::sinks::base_sink<Mutex>;

    std::array<char, rtlog_constant::OUT_BUF_SIZE> m_buf{};
    size_t m_pos{0};
    int m_stdout_fd{-1};

public:
    ColorStdoutSinkT() {
        // Open a new open file description for stdout via /proc/self/fd/1.
        // O_NONBLOCK is set only on this private fd — STDOUT_FILENO is unaffected,
        // so printf/cout/other-sinks never see EAGAIN.
        // O_APPEND ensures atomic-seek-to-EOF on each write(), preventing offset
        // corruption when stdout is redirected to a regular file.
        m_stdout_fd = ::open("/proc/self/fd/1", O_WRONLY | O_APPEND | O_NONBLOCK | O_CLOEXEC);
        if (m_stdout_fd < 0)
            m_stdout_fd = STDOUT_FILENO;  // /proc unavailable: fall back to blocking writes
    }

    ~ColorStdoutSinkT() override {
        // Switch the private fd to blocking so the final flush always completes.
        // This fcntl affects only m_stdout_fd; STDOUT_FILENO is untouched.
        if (m_stdout_fd != STDOUT_FILENO) {
            int flags = ::fcntl(m_stdout_fd, F_GETFL, 0);
            if (flags != -1)
                ::fcntl(m_stdout_fd, F_SETFL, flags & ~O_NONBLOCK);
        }
        _flush_buffer();
        if (m_stdout_fd != STDOUT_FILENO)
            ::close(m_stdout_fd);
    }

    ColorStdoutSinkT(const ColorStdoutSinkT&)            = delete;
    ColorStdoutSinkT& operator=(const ColorStdoutSinkT&) = delete;

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        // Reset before formatting so the formatter's %^ / %$ handlers always
        // write fresh values (mirrors what ansicolor_sink does in log()).
        msg.color_range_start = 0;
        msg.color_range_end   = 0;

        spdlog::memory_buf_t buf;
        Base::formatter_->format(msg, buf);

        const char* color = sink_color_for(msg.level);
        if (*color && msg.color_range_end > msg.color_range_start) {
            _buf_append(buf.data(), msg.color_range_start);
            _buf_append(color, std::strlen(color));
            _buf_append(buf.data() + msg.color_range_start, msg.color_range_end - msg.color_range_start);
            const char* reset_color = "\033[m";
            _buf_append(reset_color, std::strlen(reset_color));
            _buf_append(buf.data() + msg.color_range_end, buf.size() - msg.color_range_end);
        }
        else {
            _buf_append(buf.data(), buf.size());
        }
    }

    void flush_() override {
        _flush_buffer();
    }

private:
    void _buf_append(const char* data, size_t len) noexcept {
        if (len == 0)
            return;

        // Establish invariant: len < OUT_BUF_SIZE.
        // For messages larger than the buffer, keep only the tail (most recent bytes).
        if (len >= rtlog_constant::OUT_BUF_SIZE) {
            data += len - (rtlog_constant::OUT_BUF_SIZE - 1);
            len   = rtlog_constant::OUT_BUF_SIZE - 1;
        }

        // Make room: flush, then slide out oldest retained bytes when EAGAIN kept data.
        // If EAGAIN left data: slide guarantees m_pos + len == OUT_BUF_SIZE.
        // If flush succeeded: m_pos == 0, inner if is skipped.
        if (m_pos + len > rtlog_constant::OUT_BUF_SIZE) {
            _flush_buffer();
            if (m_pos + len > rtlog_constant::OUT_BUF_SIZE) {
                size_t discard = m_pos + len - rtlog_constant::OUT_BUF_SIZE;
                std::memmove(m_buf.data(), m_buf.data() + discard, m_pos - discard);
                m_pos -= discard;
            }
        }

        std::memcpy(m_buf.data() + m_pos, data, len);
        m_pos += len;
    }

    void _flush_buffer() noexcept {
        if (m_pos == 0)
            return;

        // O_NONBLOCK on m_stdout_fd: write() returns EAGAIN immediately when the pty
        // buffer is full → no drain thread blocking; unwritten data slides to the front
        // for next flush. STDOUT_FILENO is unaffected (remains blocking).
        size_t written = 0;
        while (written < m_pos) {
            ssize_t n = ::write(m_stdout_fd, m_buf.data() + written, m_pos - written);
            if (n < 0) {
                if (errno == EINTR)
                    continue;
                break;  // EAGAIN / EWOULDBLOCK: retry remaining bytes on next flush
            }

            if (n == 0)
                break;

            written += static_cast<size_t>(n);
        }

        // fully written — fast path
        if (written >= m_pos) {
            m_pos = 0;
            return;
        }

        // Slide unwritten bytes to buffer front
        size_t remaining = m_pos - written;
        if (remaining > 0 && written > 0)
            std::memmove(m_buf.data(), m_buf.data() + written, remaining);

        m_pos = remaining;
    }
};

using ColorStdoutSink   = ColorStdoutSinkT<spdlog::details::null_mutex>;
using ColorStdoutSinkMt = ColorStdoutSinkT<std::mutex>;

// TUI Sink: Routes spdlog messages to RtTui instead of stdout
// This sink is used when TUI mode is enabled (enableTui: true in config.yaml)
// Messages are sent to TUI's log ring buffer for display in Area 2
template<typename Mutex>
class TuiSinkT final : public spdlog::sinks::base_sink<Mutex> {
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
class BasicFileSinkT final : public spdlog::sinks::base_sink<Mutex> {
    using Base = spdlog::sinks::base_sink<Mutex>;

public:
    explicit BasicFileSinkT(const std::string& filename,
                            size_t max_size,
                            size_t max_files,
                            bool truncate = false)
        : m_fd(-1),
          m_base_filename(filename),
          m_buf_pos(0),
          m_current_size(0),
          m_max_size(max_size),
          m_max_files(max_files == 0 ? 1 : max_files) {

        _open_file(truncate);
    }

    ~BasicFileSinkT() override {
        if (m_fd >= 0) {
            _flush_buffer();
            ::close(m_fd);
        }
    }

    // Delete copy and move
    BasicFileSinkT(const BasicFileSinkT&) = delete;
    BasicFileSinkT& operator=(const BasicFileSinkT&) = delete;
    BasicFileSinkT(BasicFileSinkT&&) = delete;
    BasicFileSinkT& operator=(BasicFileSinkT&&) = delete;

    const std::string& filename() const noexcept {
        return m_base_filename;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (m_fd < 0) return;

        spdlog::memory_buf_t buf;
        Base::formatter_->format(msg, buf);

        // Check if rotation is needed
        if (m_max_size > 0 && m_current_size + buf.size() > m_max_size) {
            _rotate_files();
        }

        // If message is larger than buffer, flush current buffer and write directly
        if (buf.size() > rtlog_constant::OUT_BUF_SIZE) {
            _flush_buffer();
            ssize_t written = ::write(m_fd, buf.data(), buf.size());
            if (written > 0) {
                m_current_size += written;
            }
            return;
        }

        // If adding this message would overflow buffer, flush first
        if (m_buf_pos + buf.size() > rtlog_constant::OUT_BUF_SIZE) {
            _flush_buffer();
        }

        // Append to buffer
        std::memcpy(m_buffer.data() + m_buf_pos, buf.data(), buf.size());
        m_buf_pos += buf.size();
    }

    void flush_() override {
        _flush_buffer();
    }

private:
    void _open_file(bool truncate) noexcept {
        int flags = O_WRONLY | O_CREAT | O_CLOEXEC;
        if (truncate) {
            flags |= O_TRUNC;
            m_current_size = 0;
        } 
        else {
            flags |= O_APPEND;
            // Get current file size
            struct stat st;
            if (::stat(m_base_filename.c_str(), &st) == 0) {
                m_current_size = st.st_size;
            } 
            else {
                m_current_size = 0;
            }
        }

        m_fd = ::open(m_base_filename.c_str(), flags, 0644);
        if (m_fd < 0) {
            static const char kOpenErr[] = "[RtLog] failed to open log file\n";
            ssize_t n = ::write(STDERR_FILENO, kOpenErr, sizeof(kOpenErr) - 1);
            if (n < 0) {
                // write error is occurred but it don't needed to be reported
            }
        }
    }

    void _flush_buffer() noexcept {
        if (m_fd < 0 || m_buf_pos == 0) return;

        size_t total_written = 0;
        while (total_written < m_buf_pos) {
            ssize_t written = ::write(m_fd,
                                     m_buffer.data() + total_written,
                                     m_buf_pos - total_written);
            if (written < 0) {
                if (errno == EINTR) {
                    continue;  // Interrupted by signal - retry
                }
                // Other error: discard buffer to avoid blocking.
                // Notify via stderr (safe from noexcept, no allocation).
                static const char kWriteErr[] = "[RtLog] file write error, log data lost\n";
                ssize_t n = ::write(STDERR_FILENO, kWriteErr, sizeof(kWriteErr) - 1);
                if (n < 0) {
                    // write error is occurred but it don't needed to be reported
                }
                break;
            } 
            else if (written == 0) {
                // Disk full or quota exceeded - discard buffer
                static const char kDiskFull[] = "[RtLog] disk full, log data lost\n";
                ssize_t n = ::write(STDERR_FILENO, kDiskFull, sizeof(kDiskFull) - 1);
                if (n < 0) {
                    // write error is occurred but it don't needed to be reported
                }
                break;
            }

            total_written += written;
        }

        // Update current size with what was actually written
        if (total_written > 0) {
            m_current_size += total_written;
        }

        m_buf_pos = 0;
    }

    void _rotate_files() noexcept {
        // Flush and close current file
        _flush_buffer();
        if (m_fd >= 0) {
            ::close(m_fd);
            m_fd = -1;
        }

        // std::string operations can throw std::bad_alloc.
        // Wrap in try-catch so that a noexcept boundary violation cannot trigger std::terminate().
        try {
            for (size_t i = m_max_files - 1; i > 0; --i) {
                std::string src = m_base_filename + "." + std::to_string(i);
                std::string dst = m_base_filename + "." + std::to_string(i + 1);
                if (i == m_max_files - 1)
                    ::unlink(dst.c_str());
                ::rename(src.c_str(), dst.c_str());
            }
            std::string backup = m_base_filename + ".1";
            ::rename(m_base_filename.c_str(), backup.c_str());
        }
        catch (...) {
            // Allocation failure: skip rename chain, just open a new (truncated) file
        }

        // Open new file
        _open_file(true);
    }

private:
    int m_fd;
    std::string m_base_filename;
    std::array<char, rtlog_constant::OUT_BUF_SIZE> m_buffer{};
    size_t m_buf_pos;
    size_t m_current_size;
    size_t m_max_size;
    size_t m_max_files;
};

using BasicFileSink   = BasicFileSinkT<spdlog::details::null_mutex>;
using BasicFileSinkMt = BasicFileSinkT<std::mutex>;

class RtLog {
public:
    using log_level = spdlog::level::level_enum;
    // Increased capacity from 256 to 2048 to handle high-frequency burst logging
    // if system generates total message per ~3400 msg/sec
    // With 2048 capacity, can buffer ~600ms worth of messages during drain delays
    using QueueType = LogQueue<rtlog_constant::QUEUE_CAPACITY, rtlog_constant::QUEUE_MSG_LEN>;
    using Entry = QueueType::Entry;

    struct TimeBase {
        int64_t wall_ns;      // CLOCK_REALTIME
        int64_t monotonic_ns; // CLOCK_MONOTONIC — must match the clock used in _mono_now_ns()

        // need to be called in nonRT before log task is starting
        static TimeBase capture() noexcept {
            TimeBase tb{};
            struct timespec tw{}, tm{};
            clock_gettime(CLOCK_REALTIME, &tw);
            clock_gettime(CLOCK_MONOTONIC, &tm);  // must match _mono_now_ns()
            tb.wall_ns = static_cast<int64_t>(tw.tv_sec) * 1'000'000'000LL + tw.tv_nsec;
            tb.monotonic_ns = static_cast<int64_t>(tm.tv_sec) * 1'000'000'000LL + tm.tv_nsec;
            return tb;
        }

        // convert monotonic to wall clock
        int64_t to_wall_ns(int64_t mono_ns) const noexcept {
            return wall_ns + (mono_ns - monotonic_ns);
        }
    };

public:
    static RtLog& instance() noexcept {
        static RtLog s_instance;
        return s_instance;
    }

    static void Initialize(const std::string& log_name, const std::string& file_basename = "", bool enable_tui = false, bool annot_datetime = true, bool truncate = false, 
                        size_t max_file_size = rtlog_constant::DEFAULT_MAX_SIZE, size_t max_files = rtlog_constant::DEFAULT_MAX_FILES) {
        auto& s_instance = instance();
        
        // create spdlog logger with appropriate sinks
        s_instance.m_logger = std::make_shared<spdlog::logger>(log_name);
        if (s_instance.m_logger) {
            s_instance.m_logger->sinks().clear();

            // create tui instance if enabled
            if (enable_tui) {
                s_instance.m_tui = std::make_shared<Utils::RtTui>();
                if (s_instance.m_tui->init()) {
                    auto tui_sink = std::make_shared<TuiSink>(s_instance.m_tui);
                    tui_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
                    s_instance.m_logger->sinks().push_back(tui_sink);
                }
                else {
                    // TUI init failed: fall back to colored stdout
                    s_instance.m_tui.reset();
                    auto console_sink = std::make_shared<ColorStdoutSinkMt>();
                    console_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
                    s_instance.m_logger->sinks().push_back(console_sink);
                    RtLog::log_raw(RtLog::log_level::err, "TUI initialize failed -> use default stdout");
                }
            }
            // create default color stdout sink
            else {
                auto console_sink = std::make_shared<ColorStdoutSinkMt>();
                console_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
                s_instance.m_logger->sinks().push_back(console_sink);
            }
        }

        if (!file_basename.empty() && (file_basename != "_STDOUT_")) {
            spdlog::filename_t filename = file_basename;
            if (annot_datetime) {
                filename = s_instance._annotate_filename_datetime(file_basename);
                auto [dname, fname] = s_instance._split_by_directory(filename);
                (void)remove(file_basename.c_str());
                auto rtn = symlink(fname.c_str(), file_basename.c_str());
                if (rtn < 0) {
                    // Cannot create symlink to this log file. Log a warning to the console sink if available.
                    if (s_instance.m_logger) {
                        s_instance.m_logger->log(spdlog::level::warn,
                            "Cannot create symlink '{}' → '{}': {}", file_basename, fname, strerror(errno));
                    }
                }
            }

            auto file_sink = std::make_shared<BasicFileSinkMt>(filename, max_file_size, max_files, truncate);
            file_sink->set_pattern("%^[%L][%H:%M:%S.%f]%$ %v");
            s_instance.m_logger->sinks().push_back(file_sink);
        }

        spdlog::set_default_logger(s_instance.m_logger);
        s_instance.m_timebase = TimeBase::capture();
        s_instance.m_initialized.store(true, std::memory_order_release);
    }

    /**
     * Terminate logging system
     */
    static void Terminate() {
        auto& s_instace = instance();

        // Report any messages that were dropped during operation
        uint64_t drops = s_instace.drop_count();
        if (drops > 0)
            s_instace.m_logger->log(spdlog::level::warn, "CloseLogger: RT log queue dropped %llu messages during operation", (unsigned long long)drops);

        s_instace.drain_all();

        // Stop TUI if enabled
        if (s_instace.m_tui) {
            s_instace.m_tui->stop();
            s_instace.m_tui.reset();
        }

        s_instace.m_initialized.store(false, std::memory_order_release);
        
        // flush all pending log messages
        spdlog::shutdown();
    }

    /**
     * Set log level of default logger
     * @param lvl log level
     */
    static void SetLogLevel(log_level lvl) {
        instance().set_level(lvl);
    }

    // Get TUI instance (for external use)
    std::shared_ptr<Utils::RtTui> get_tui() const noexcept {
        return m_tui;
    }

    // Check if TUI mode is enabled
    bool is_tui_mode() const noexcept {
        return m_tui != nullptr;
    }

    // Get the latest pending key from TUI (non-blocking, returns 0 if no key)
    char get_pending_key() const noexcept {
        if (m_tui != nullptr) {
            return m_tui->pop_pendingkey();
        }
        
        return 0;
    }

    // TUI Area 1 helpers (RT-safe, only if TUI enabled)
    // layout_idx: 0 ~ MAX_LAYOUTS-1, group_idx: 0 ~ TUI_MAX_GROUPS-1, row_idx: 0 ~ TUI_MAX_ROWS_PER_GROUP-1
    template<typename... Args>
    void tui_set_row(int layout_idx, int group_idx, int row_idx,
                     const char* label, const char* format, Args... args) noexcept {
        if (m_tui) {
            m_tui->set_row_fmt(layout_idx, group_idx, row_idx, label, format, args...);
        }
    }

    template<typename... Args>
    void tui_set_row_v(int layout_idx, int group_idx, int row_idx,
                       const char* label, Args... args) noexcept {
        if (m_tui) {
            m_tui->set_row_v(layout_idx, group_idx, row_idx, label, args...);
        }
    }

    template<typename... Args>
    void tui_set_group(int layout_idx, int group_idx, const char* label, Args... args) noexcept {
        if (m_tui) {
            m_tui->set_group_v(layout_idx, group_idx, label, args...);
        }
    }

    void tui_set_text_row(int layout_idx, int group_idx, int row_idx,
                          const char* label, const char* text) noexcept {
        if (m_tui) {
            m_tui->set_text_row(layout_idx, group_idx, row_idx, label, text);
        }
    }

    // printf-style text row: formats args internally, then calls set_text_row.
    // Example: tui_set_text_row_fmt(0, 2, 0, "Right",
    //              "Pos:%+8.3f,%+8.3f,%+8.3f   Vel:%+8.3f,%+8.3f,%+8.3f",
    //              px, py, pz, vx, vy, vz);
    void tui_set_text_row_fmt(int layout_idx, int group_idx, int row_idx,
                              const char* label, const char* fmt, ...) noexcept
        __attribute__((format(printf, 6, 7))) {
        if (!m_tui)
            return;

        char buf[Utils::RtTui::TUI_TEXT_ROW_LEN];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        m_tui->set_text_row(layout_idx, group_idx, row_idx, label, buf);
    }

    void tui_set_layout_name(int layout_idx, const char* name) noexcept {
        if (m_tui) {
            m_tui->set_layout_name(layout_idx, name);
        }
    }

    int tui_get_layout() const noexcept {
        if (m_tui)
            return m_tui->get_current_layout();

        return 0;
    }

    bool is_initialized() const noexcept {
        return m_initialized.load(std::memory_order_acquire);
    }

    // Called repeatedly by the log thread in a loop.
    // Sleeps POLL_INTERVAL_NS then drains all queued entries.
    // No syscall occurs in the RT producer path.
    static void thread_entry(void* /*pArg*/) noexcept {
        instance()._thread_entry();
    }

    void refresh_timebase() noexcept {
        m_timebase = TimeBase::capture();
    }

    // Flush all remaining entries from the RT queue.
    // IMPORTANT: must only be called from a single consumer thread at a time.
    // LogQueue is MPSC — concurrent try_pop() from two threads is undefined behaviour.
    // Callers must ensure the drain thread has stopped before calling this (e.g. in Terminate()).
    size_t drain_all() noexcept {
        size_t count = 0;
        Entry entry;
        while (m_queue.try_pop(entry)) {
            _flush_entry(entry);
            ++count;
        }

        return count;
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
    static void log_raw(log_level lvl, const char* fmt, ...) noexcept {
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
        // sink_color_for() is defined in the same dt namespace, no allocation
        const char* color = sink_color_for(lvl);
        int hdr = snprintf(buf, sizeof(buf),
                           "%s[%c][%02d:%02d:%02d.%06d]\033[0m\033[1m[RAW]\033[0m ",
                           color, lc, hh, mm, ss, us);
        if (hdr < 0 || hdr >= (int)sizeof(buf))
            hdr = 0;

        if (!fmt)
            fmt = "(null)";

        // Message
        va_list args;
        va_start(args, fmt);
        int msg_n = vsnprintf(buf + hdr, sizeof(buf) - (size_t)hdr - 1, fmt, args);
        va_end(args);
        if (msg_n < 0) msg_n = 0;

        size_t total = (size_t)hdr + (size_t)msg_n;
        if (total >= sizeof(buf) - 1)
            total = sizeof(buf) - 2;
        buf[total++] = '\n';

        // Atomic write: messages ≤ PIPE_BUF (4096 B) are never interleaved on Linux
        ssize_t n = ::write(STDERR_FILENO, buf, total);
        if (n < 0) {
            // write error is occurred but it don't needed to be reported
        }
    }

    template<typename... Args>
    void log_rt(log_level lvl, const char* format, Args... args) noexcept {
        if (!m_initialized.load(std::memory_order_acquire))
            return;

        Entry entry;
        entry.set(lvl, _mono_now_ns(), format, args...);
        _enqueue(entry);
    }

    void set_level(log_level lvl) noexcept {
        m_level.store(static_cast<int>(lvl), std::memory_order_relaxed);
        // Synchronize with spdlog logger level
        if (m_logger) {
            m_logger->set_level(lvl);
        }
    }

    log_level get_level() const noexcept {
        return static_cast<log_level>(m_level.load(std::memory_order_relaxed));
    }

    uint64_t drop_count() const noexcept {
        return m_drop_count.load(std::memory_order_relaxed);
    }

    // Get approximate number of messages currently in queue
    size_t queue_size() const noexcept {
        return m_queue.approx_size();
    }

    // Get queue utilization percentage (0-100)
    size_t queue_utilization() const noexcept {
        size_t size = m_queue.approx_size();
        return (size * 100) / QueueType::capacity();
    }

    // Get queue statistics
    struct QueueStats {
        size_t current_size;      // Current number of messages in queue
        size_t capacity;          // Maximum queue capacity
        size_t utilization_pct;   // Utilization percentage (0-100)
        uint64_t total_drops;     // Total number of dropped messages
    };

    QueueStats get_queue_stats() const noexcept {
        size_t size = m_queue.approx_size();
        return QueueStats{
            .current_size = size,
            .capacity = QueueType::capacity(),
            .utilization_pct = (size * 100) / QueueType::capacity(),
            .total_drops = m_drop_count.load(std::memory_order_relaxed)
        };
    }

    class [[nodiscard]] LogRtStream {
    public:
        static constexpr size_t BUF_LEN = QueueType::msg_len();
                
        LogRtStream(log_level lvl) noexcept:
            m_log_level(lvl),
            m_active(RtLog::instance()._is_active_level(lvl)),
            m_pos(0)
        {
            m_buf[0] = '\0';
        }

        // prevent copy and move
        LogRtStream(const LogRtStream&)            = delete;
        LogRtStream& operator=(const LogRtStream&) = delete;
        LogRtStream(LogRtStream&&)                 = delete;
        LogRtStream& operator=(LogRtStream&&)      = delete;

        ~LogRtStream() noexcept {
            if (m_active && m_pos > 0)
                RtLog::instance().log_rt(m_log_level, "%.*s", (int)m_pos, m_buf);
        }

        template<typename T, typename = std::enable_if_t<
            std::is_arithmetic<T>::value ||
            std::is_pointer<T>::value ||
            std::is_enum<T>::value>>
        LogRtStream& operator<<(T value) noexcept;

        template <typename T>
        LogRtStream& operator<<(const std::vector<T>& value) noexcept;

        // Eigen dense vector/matrix support — implementation in dtRtLogEigen.hpp
        // Include dtRtLogEigen.hpp (not this header) in files that use Eigen types.
        template<typename Derived>
        LogRtStream& operator<<(const Eigen::MatrixBase<Derived>& vec) noexcept;

        // dt::Math vector support — implementation in dtRtLogDtMath.hpp
        // Include dtRtLogDtMath.hpp (not this header) in files that use dt::Math vectors.
        template<uint16_t N, typename T>
        LogRtStream& operator<<(const Math::Vector<N, T>& vec) noexcept;
        template<typename T, uint16_t N>
        LogRtStream& operator<<(const Math::Vector3<T, N>& vec) noexcept;
        template<typename T, uint16_t N>
        LogRtStream& operator<<(const Math::Vector4<T, N>& vec) noexcept;
        template<typename T, uint16_t N>
        LogRtStream& operator<<(const Math::Vector6<T, N>& vec) noexcept;
        template<typename T>
        LogRtStream& operator<<(const Math::VectorX<T>& vec) noexcept;

        LogRtStream& operator<<(const char* str) noexcept;
        LogRtStream& operator<<(const std::string& str) noexcept;
        LogRtStream& operator<<(char c) noexcept;

        // fmt-style format(), compatible with dtLog::LogStream::format().
        // Uses fmt::format_to_n() which writes directly to the stack buffer —
        // no heap allocation for primitive types (int, double, const char*, etc.).
        // dt::Math::Vector / Eigen types have no fmt::formatter: use operator<< instead.
        template<typename... Args>
        LogRtStream& format(fmt::format_string<Args...> fmt_str, Args&&... args) noexcept {
            if (!m_active) 
                return *this;

            size_t avail = BUF_LEN - 1 - m_pos;
            if (avail == 0) { 
                _add_truncation(); return *this; 
            }

            try {
                auto result = fmt::format_to_n(m_buf + m_pos, avail, fmt_str, std::forward<Args>(args)...);
                m_pos += result.size < avail ? result.size : avail;
                m_buf[m_pos] = '\0';
            }
            catch (...) {
                _add_truncation();
            }

            return *this;
        }

    private:
        void _append(const char* src, size_t len) noexcept {
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
        bool _format_element(T value) noexcept {
            // Ensure we have space for at least "..." if truncation needed
            if (m_pos + 10 >= BUF_LEN)
                return false;

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
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%lld", static_cast<long long>(value));
            else
                written = std::snprintf(&m_buf[m_pos], BUF_LEN - m_pos, "%llu", static_cast<unsigned long long>(value));

            // Check if snprintf failed or buffer was insufficient
            if (written < 0 || written >= static_cast<int>(BUF_LEN - m_pos))
                return false;

            m_pos += written;

            // Always maintain null terminator
            if (m_pos < BUF_LEN)
                m_buf[m_pos] = '\0';

            return true;
        }

        // Helper to add truncation indicator
        void _add_truncation() noexcept {
            if (m_pos + 3 < BUF_LEN) {
                m_buf[m_pos++] = '.';
                m_buf[m_pos++] = '.';
                m_buf[m_pos++] = '.';
                m_buf[m_pos] = '\0';
            }
        }

        // Helper to add separator ", "
        bool _add_separator() noexcept {
            if (m_pos + 2 < BUF_LEN) {
                m_buf[m_pos++] = ',';
                m_buf[m_pos++] = ' ';
                return true;
            }
            return false;
        }

        // Helper to close array with ']'
        void _close_array() noexcept {
            if (m_pos + 1 < BUF_LEN) {
                m_buf[m_pos++] = ']';
                m_buf[m_pos] = '\0';
            }
        }

    private:
        log_level m_log_level;
        bool m_active;
        size_t m_pos;
        char m_buf[BUF_LEN];
    };

private:
    RtLog() noexcept:
        m_logger(nullptr),
        m_timebase{},
        m_initialized(false)
    {
        m_level.store(static_cast<int>(log_level::trace), std::memory_order_relaxed);
        m_drop_count.store(0, std::memory_order_relaxed);
    }

    ~RtLog() = default;

    // prevent copy and move
    RtLog(const RtLog&)            = delete;
    RtLog& operator=(const RtLog&) = delete;
    RtLog(RtLog&&)                 = delete;
    RtLog& operator=(RtLog&&)      = delete;

    void _thread_entry() noexcept {
        // Sleep in the non-RT log thread, then drain all queued entries.
        // The RT producer path is syscall-free: it only writes to the lock-free queue.

        // Check queue size BEFORE draining to determine next polling interval
        size_t queue_size_before = m_queue.approx_size();

        // Drain all queued entries (TuiSinkT pushes to TUI queue here)
        size_t count = drain_all();

        // Rate-limit flush() to 100 ms regardless of message count.
        // Flushing even when count == 0 drains EAGAIN-retained bytes left in the sink buffer,
        // preventing the last few messages before a quiet period from being stuck.
        const int64_t now_ns = _mono_now_ns();
        if (m_logger && now_ns - m_last_flush_ns >= 100'000'000LL) {  // 100 ms
            m_logger->flush();
            m_last_flush_ns = now_ns;
        }

        // TUI tick: 25 Hz (40 ms) rate-limiter — drains queue, handles keys, renders
        if (m_tui) {
            if (now_ns - m_tui_last_render_ns >= 40'000'000LL) {
                m_tui->tick();
                m_tui_last_render_ns = now_ns;
            }
        }

        // Check queue size AFTER draining to see if we're keeping up
        size_t queue_size_after = m_queue.approx_size();

        // Adaptive polling: adjust interval based on queue pressure
        long interval_ns;

        if (queue_size_after > QueueType::capacity() / 2) {
            interval_ns = 100'000L;   // 100 μs: queue >50% — drain as fast as possible
        }
        else if (count > 0 && queue_size_after >= queue_size_before) {
            interval_ns = 100'000L;  // 100 μs: queue not shrinking after drain (falling behind)
        }
        else if (count > 0) {
            interval_ns = 500'000L;  // 500 μs: draining normally
        }
        else {
            interval_ns = rtlog_constant::POLL_INTERVAL_NS;  // 1 ms: idle (avoids 0>=0 false-positive)
        }

        struct timespec ts{0, interval_ns};
        clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, nullptr);
    }

    void _flush_entry(const Entry& entry) noexcept {
        if (!m_logger)
            return;

        // spdlog sinks allocate heap memory for formatting (spdlog::memory_buf_t).
        // An std::bad_alloc thrown inside this noexcept function would call std::terminate().
        try {
            auto wall_ns = m_timebase.to_wall_ns(entry.timestamp_ns);
            auto duration = std::chrono::nanoseconds(wall_ns);
            auto tp = spdlog::log_clock::time_point(
                std::chrono::duration_cast<spdlog::log_clock::duration>(duration)
            );

            m_logger->log(
                tp,
                spdlog::source_loc{},
                entry.level,
                spdlog::string_view_t(entry.msg, entry.msg_len)
            );
        }
        catch (...) {
            // Formatting failed (likely std::bad_alloc). Increment drop counter so
            // Terminate() reports the true number of lost entries.
            m_drop_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void _enqueue(const Entry& entry) noexcept {
        // RT-safe: lock-free push only, no semaphore post, no syscall
        if (!m_queue.try_push(entry)) {
            // Only increment the drop counter; pushing into a full queue is pointless.
            // Monitor via drop_count() or display with TUI_SET_ROW_V.
            m_drop_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    bool _is_active_level(log_level lvl) const noexcept {
        return static_cast<int>(lvl) >= m_level.load(std::memory_order_relaxed);
    }

    inline int64_t _mono_now_ns() const noexcept {
        struct timespec ts;
        // CLOCK_MONOTONIC is intercepted by Xenomai POSIX skin and runs in primary
        // mode. CLOCK_MONOTONIC_RAW is a Linux-only clock that falls through to the
        // Linux kernel, triggering a primary -> secondary mode switch on every call.
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return static_cast<int64_t>(ts.tv_sec) * 1'000'000'000LL + static_cast<int64_t>(ts.tv_nsec);
    }

    std::string _annotate_filename_datetime(const std::string& file_basename) {
        spdlog::filename_t filename;

        time_t tnow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        tm now_tm = spdlog::details::os::localtime(tnow);
        
        auto [basename, ext] = spdlog::details::file_helper::split_by_extension(file_basename);

        filename = fmt::format(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}{}"), basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1,
            now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ext);

        return filename;
    }

    std::tuple<std::string, std::string> _split_by_directory(const std::string &fname) {
        auto dir_index = fname.rfind('/');

        // no valid directory found - return empty string as folder and whole path
        if (dir_index == std::string::npos)
            return {std::string(), fname};

        // ends up with '/' - return whole path as directory and empty string as filename
        if (dir_index == fname.size() - 1)
            return {fname, std::string()};

        // finally - return a valid directory and file path tuple
        return {fname.substr(0, dir_index + 1), fname.substr(dir_index + 1)};   // '/' is included as directory name
    }

private:
    std::shared_ptr<spdlog::logger> m_logger;
    std::shared_ptr<Utils::RtTui> m_tui;   // TUI instance (if enabled)
    int64_t               m_tui_last_render_ns{0};  // last TUI render timestamp (25 Hz rate-limiter)
    int64_t               m_last_flush_ns{0};        // last spdlog flush timestamp (100 ms rate-limiter)
    QueueType             m_queue;
    TimeBase              m_timebase;
    std::atomic<bool>     m_initialized;
    std::atomic<int>      m_level;
    std::atomic<uint64_t> m_drop_count;
};  // class RtLog
};  // namespace dt

// LOG_RT(level): returns a LogRtStream temporary — identical usage to dtLog's LOG(level).
//   LOG_RT(info) << "msg: " << val;
//   LOG_RT(info).format("x={:.3f} idx={}", x, idx);
//   LOG_RT(warn) << "q=" << q;   // dt::Math::Vector / Eigen: use operator<<, not format()
#define LOG_RT(level) \
    dt::RtLog::LogRtStream(dt::RtLog::log_level::level)
#define LOG(level) \
    dt::RtLog::LogRtStream(dt::RtLog::log_level::level)

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
    dt::RtLog::log_raw(dt::RtLog::log_level::level, fmt, ##__VA_ARGS__)

#define LOG_RT_RAW_INFO(fmt, ...)  LOG_RT_RAW(info,     fmt, ##__VA_ARGS__)
#define LOG_RT_RAW_WARN(fmt, ...)  LOG_RT_RAW(warn,     fmt, ##__VA_ARGS__)
#define LOG_RT_RAW_ERR(fmt, ...)   LOG_RT_RAW(err,      fmt, ##__VA_ARGS__)
#define LOG_RT_RAW_CRIT(fmt, ...)  LOG_RT_RAW(critical, fmt, ##__VA_ARGS__)

// ═══════════════════════════════════════════════════════════════════════════
// TUI Area 1 macros (RT-safe, no-op if TUI disabled)
// ═══════════════════════════════════════════════════════════════════════════
//
// Layout 0 (default) — backward-compatible macros
// ─────────────────────────────────────────────────
// Format-based row update
// Example: TUI_SET_ROW(grp, row, "label", "%.2f", v1, v2, v3)
#define TUI_SET_ROW(grp, row, label, fmt, ...) \
    dt::RtLog::instance().tui_set_row(0, grp, row, label, fmt, ##__VA_ARGS__)

// Auto-format row update (type-deduced)
// Example: TUI_SET_ROW_V(grp, row, "label", v1, v2, v3)
#define TUI_SET_ROW_V(grp, row, label, ...) \
    dt::RtLog::instance().tui_set_row_v(0, grp, row, label, ##__VA_ARGS__)

// Full-width text row (ignores column layout, max 200 chars)
// Example: TUI_SET_TEXT_ROW(grp, row, "cmd", cmd_str.c_str())
#define TUI_SET_TEXT_ROW(grp, row, label, text) \
    dt::RtLog::instance().tui_set_text_row(0, grp, row, label, text)

// Group header setup (call once at startup for layout 0)
// Example: TUI_SET_GROUP(grp, "Joint", "J1", "J2", "J3")
#define TUI_SET_GROUP(grp, label, ...) \
    dt::RtLog::instance().tui_set_group(0, grp, label, ##__VA_ARGS__)

//
// Layout-aware macros — specify layout_idx (0-based) as first argument
// ─────────────────────────────────────────────────────────────────────
// Example: TUI_L_SET_ROW(1, grp, row, "label", "%.2f", v1, v2)  → layout 1
#define TUI_L_SET_ROW(layout, grp, row, label, fmt, ...) \
    dt::RtLog::instance().tui_set_row(layout, grp, row, label, fmt, ##__VA_ARGS__)

#define TUI_L_SET_ROW_V(layout, grp, row, label, ...) \
    dt::RtLog::instance().tui_set_row_v(layout, grp, row, label, ##__VA_ARGS__)

#define TUI_L_SET_TEXT_ROW(layout, grp, row, label, text) \
    dt::RtLog::instance().tui_set_text_row(layout, grp, row, label, text)

// printf-style text row variants (format string + args)
// Example: TUI_SET_TEXT_ROW_FMT(grp, row, "Right",
//              "Pos:%+8.3f,%+8.3f,%+8.3f   Vel:%+8.3f,%+8.3f,%+8.3f", px,py,pz, vx,vy,vz)
#define TUI_SET_TEXT_ROW_FMT(grp, row, label, fmt, ...) \
    dt::RtLog::instance().tui_set_text_row_fmt(0, grp, row, label, fmt, ##__VA_ARGS__)

#define TUI_L_SET_TEXT_ROW_FMT(layout, grp, row, label, fmt, ...) \
    dt::RtLog::instance().tui_set_text_row_fmt(layout, grp, row, label, fmt, ##__VA_ARGS__)

// Group header setup for a specific layout (call once at startup)
// Example: TUI_L_SET_GROUP(1, grp, "TaskState", "X", "Y", "Z")
#define TUI_L_SET_GROUP(layout, grp, label, ...) \
    dt::RtLog::instance().tui_set_group(layout, grp, label, ##__VA_ARGS__)

// Set the display name of a layout (shown in the bottom status bar)
// Example: TUI_SET_LAYOUT_NAME(0, "Overview")
#define TUI_SET_LAYOUT_NAME(layout, name) \
    dt::RtLog::instance().tui_set_layout_name(layout, name)

// Query the currently active layout index (0-based)
#define TUI_GET_LAYOUT()        dt::RtLog::instance().tui_get_layout()

#define TUI_GET_PENDING_KEY()   dt::RtLog::instance().get_pending_key()
#define TUI_IS_ENABLED()        dt::RtLog::instance().is_tui_mode()

// ═══════════════════════════════════════════════════════════════════════════
// TuiSinkT Implementation
// ═══════════════════════════════════════════════════════════════════════════
namespace dt {

template<typename Mutex>
void TuiSinkT<Mutex>::sink_it_(const spdlog::details::log_msg& msg) {
    if (!m_tui) 
        return;

    msg.color_range_start = 0;
    msg.color_range_end   = 0;

    spdlog::memory_buf_t buf;
    Base::formatter_->format(msg, buf);

    size_t sz = buf.size();
    if (sz > 0 && buf[sz - 1] == '\n') 
        sz--;

    const char* color = sink_color_for(msg.level);
    if (*color && msg.color_range_end > msg.color_range_start && msg.color_range_end <= sz) {
        // Color applied to prefix ([L][timestamp]) only; message body uses default color.
        // Assembled in a stack buffer (no heap allocation) then pushed to the TUI queue.
        static constexpr size_t TMP_LEN = rtlog_constant::QUEUE_MSG_LEN;
        char tmp[TMP_LEN];
        size_t pos = 0;

        auto safe_copy = [&](const char* src, size_t len) {
            size_t n = std::min(len, TMP_LEN - 1 - pos);
            if (n) {
                std::memcpy(tmp + pos, src, n); pos += n;
            }
        };

        safe_copy(buf.data(), msg.color_range_start);                              // before color range
        safe_copy(color, std::strlen(color));                                      // color code
        safe_copy(buf.data() + msg.color_range_start, msg.color_range_end - msg.color_range_start); // colored prefix
        safe_copy("\033[m", 3);                                                    // reset
        safe_copy(buf.data() + msg.color_range_end, sz - msg.color_range_end);     // message body

        tmp[pos] = '\0';
        m_tui->log(msg.level, "%s", tmp);
    }
    else {
        m_tui->log(msg.level, "%.*s", (int)sz, buf.data());
    }
}

template<typename T, typename = std::enable_if_t<
    std::is_arithmetic<T>::value ||
    std::is_pointer<T>::value ||
    std::is_enum<T>::value>>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(T value) noexcept {
    if (!m_active)
        return *this;

    if (!_format_element(value)) {
        _add_truncation();
    }

    return *this;
}

template <typename T>
RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const std::vector<T>& value) noexcept {
    if (!m_active)
        return *this;

    // Open array bracket
    if (m_pos + 1 >= BUF_LEN)
        return *this;
    m_buf[m_pos++] = '[';

    const size_t count = value.size();
    for (size_t i = 0; i < count; i++) {
        // Format current element
        if (!_format_element(value[i])) {
            _add_truncation();
            break;
        }

        // Add separator if not the last element
        if (i != count - 1) {
            if (!_add_separator())
                break;
        }
    }

    _close_array();
    return *this;
}

inline RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const char* str) noexcept {
    if (m_active && str)
        _append(str, strnlen(str, BUF_LEN));
    return *this;
}

inline RtLog::LogRtStream& RtLog::LogRtStream::operator<<(const std::string& str) noexcept {
    if (m_active)
        _append(str.c_str(), str.size());
    return *this;
}

inline RtLog::LogRtStream& RtLog::LogRtStream::operator<<(char c) noexcept {
    if (m_active && m_pos + 1 < BUF_LEN) {
        m_buf[m_pos++] = c;
        m_buf[m_pos] = '\0';
    }
    return *this;
}

}  // namespace dt
