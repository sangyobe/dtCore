/*!
 \file      dtLogQueue.hpp
 \brief     MPSC queue for logging
 \author    myungjin.kim@hyundai.com
 \date      2026. 4. 24
 \version   0.0.1
 \copyright RoboticsLab ART All rights reserved.
*/

#pragma once
#include <spdlog/common.h>
#include <cstdint>
#include <atomic>
#include <array>
#include <cstdarg>
#include <cstdio>
#include <algorithm>

// MPSC (Multi-Producer Single-Consumer) Ring Buffer
// - Multi RT or nonRT task → Producer
// - Single logging task → Consumer
// - lock-free, no syscall
template<size_t CAPACITY = 32, size_t MSG_LEN = 256>
class LogQueue {
    static_assert((CAPACITY & (CAPACITY - 1)) == 0, "CAPACITY must be power of 2");
    static_assert(MSG_LEN >= 256 && MSG_LEN <= 4096, "MSG_LEN out of range (256 <= MSG_LEN <= 4096)");

public:
    using log_level = spdlog::level::level_enum;

    // LOG message structure
    struct Entry {
        // When enqueue is called, Should enter the latest timestamp value obtained without a syscall
        int64_t   timestamp_ns{0};
        log_level level{log_level::info};
        size_t    msg_len{0};
        char      msg[MSG_LEN]{};

        // set message - template version (C++ style)
        template<typename... Args>
        void set(log_level lvl, int64_t ts_ns, const char* format, Args... args) noexcept {
            level        = lvl;
            timestamp_ns = ts_ns;
            // Suppress -Wformat-security warning: format string comes from LOG_RT macro,
            // which is always a string literal in user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
            int n  = std::snprintf(msg, MSG_LEN, format, args...);
#pragma GCC diagnostic pop
            msg_len = (n > 0) ? static_cast<size_t>(std::min(n, (int)MSG_LEN - 1)) : 0;
        }

        // set message - va_list version (for internal use)
        void set_v(log_level lvl, int64_t ts_ns, const char* format, va_list args) noexcept {
            level        = lvl;
            timestamp_ns = ts_ns;
            int n   = std::vsnprintf(msg, MSG_LEN, format, args);
            msg_len = (n > 0) ? static_cast<size_t>(std::min(n, (int)MSG_LEN - 1)) : 0;
        }
    };

    LogQueue() noexcept {
        for (size_t i = 0; i < CAPACITY; ++i)
            slots_[i].seq.store(static_cast<uint32_t>(i), std::memory_order_relaxed);

        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed);
    }

    // remove copy / move operator
    LogQueue(const LogQueue&) = delete;
    LogQueue& operator= (const LogQueue&) = delete;
    LogQueue(LogQueue&&) = delete;
    LogQueue& operator= (LogQueue&&) = delete;

    /**
     * @brief push log message into MPSC queue (for producer)
     *
     * @param Entry: log message to push
     * @return bool: if queue is full, then returns false. Otherwise, it returns true.
     */
    bool try_push(const Entry& entry) noexcept {
        uint32_t head = head_.load(std::memory_order_relaxed);

        for (;;) {
            Slot& slot = slots_[head & (CAPACITY - 1)];
            uint32_t seq = slot.seq.load(std::memory_order_acquire);
            int32_t diff = (int32_t)seq - (int32_t)head;

            if (diff == 0) {
                // CAS (Compare And Swap)
                if (head_.compare_exchange_strong(head, head + 1, std::memory_order_relaxed, std::memory_order_relaxed)) {
                    slot.data = entry;
                    slot.seq.store(head + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                // queue full
                return false;
            }
            else {
                // diff > 0: head is stale -> need to reload and re-execute
                head = head_.load(std::memory_order_relaxed);
            }
        }
    }

    /**
     * @brief pop log message from MPSC queue (for consumer)
     *
     * @param Entry: output log message data
     * @return bool: if queue is empty, then it returns false. Otherwise, it returns true.
     */
    bool try_pop(Entry& out) noexcept {
        uint32_t tail = tail_.load(std::memory_order_relaxed);
        Slot&    slot = slots_[tail & (CAPACITY - 1)];
        uint32_t seq  = slot.seq.load(std::memory_order_acquire);
        int32_t  diff = (int32_t)seq - (int32_t)(tail + 1);

        if (diff == 0) {
            out = slot.data;
            tail_.store(tail + 1, std::memory_order_relaxed);
            slot.seq.store(tail + CAPACITY, std::memory_order_release);
            return true;
        }
        return false;
    }

    // only use for hint
    bool is_empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    // Approximate size of queue (number of messages currently in queue)
    // This is an approximation due to concurrent access by multiple producers
    // Only use for monitoring/statistics, not for critical decisions
    size_t approx_size() const noexcept {
        uint32_t h = head_.load(std::memory_order_acquire);
        uint32_t t = tail_.load(std::memory_order_acquire);
        // Wrapping subtraction handles uint32_t overflow correctly
        return static_cast<size_t>(h - t);
    }

    static constexpr size_t capacity() noexcept { return CAPACITY; }
    static constexpr size_t msg_len() noexcept { return MSG_LEN; }

private:
    struct Slot {
        std::atomic<uint32_t> seq{0};   // queue number
        Entry                 data;
    };

    alignas(64) std::atomic<uint32_t> head_{0};
    alignas(64) std::atomic<uint32_t> tail_{0};
    std::array<Slot, CAPACITY>        slots_;
};
