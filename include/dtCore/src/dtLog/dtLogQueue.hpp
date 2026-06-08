/*!
 \file      dtLogQueue.hpp
 \brief     MPSC queue for logging
 \author    myungjin.kim@hyundai.com
 \date      2026. 4. 24
 \version   0.0.1
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef _DT_LOG_QUEUE_H_
#define _DT_LOG_QUEUE_H_

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
template<size_t m_capacity = 32, size_t m_msgLen = 256>
class LogQueue 
{
    static_assert((m_capacity & (m_capacity - 1)) == 0, "m_capacity must be power of 2");
    static_assert(m_msgLen >= 256 && m_msgLen <= 4096, "m_msgLen out of range (256 <= m_msgLen <= 4096)");

public:
    using log_level = spdlog::level::level_enum;

    // LOG message structure
    struct Entry 
    {
        // When enqueue is called, Should enter the latest timestamp value obtained without a syscall
        int64_t   timeStamp_ns{0};
        log_level level{log_level::info};
        size_t    msgLen{0};
        char      msg[m_msgLen]{};

        // set message - template version (C++ style)
        template<typename... Args>
        void Set(log_level lvl, int64_t ts_ns, const char* format, Args... args) noexcept 
        {
            level        = lvl;
            timeStamp_ns = ts_ns;
            // Suppress -Wformat-security warning: format string comes from LOG_RT macro,
            // which is always a string literal in user code
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
            int n  = std::snprintf(msg, m_msgLen, format, args...);
#pragma GCC diagnostic pop
            msgLen = (n > 0) ? static_cast<size_t>(std::min(n, (int)m_msgLen - 1)) : 0;
        }

        // set message - va_list version (for internal use)
        void SetV(log_level lvl, int64_t ts_ns, const char* format, va_list args) noexcept
        {
            level        = lvl;
            timeStamp_ns = ts_ns;
            int n  = std::vsnprintf(msg, m_msgLen, format, args);
            msgLen = (n > 0) ? static_cast<size_t>(std::min(n, (int)m_msgLen - 1)) : 0;
        }
    };

    LogQueue() noexcept 
    {
        for (size_t i = 0; i < m_capacity; ++i)
        {
            m_slots[i].seq.store(static_cast<uint32_t>(i), std::memory_order_relaxed);
        }

        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
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
    bool TryPush(const Entry& entry) noexcept 
    {
        uint32_t head = m_head.load(std::memory_order_relaxed);

        for (;;) 
        {
            Slot& slot = m_slots[head & (m_capacity - 1)];
            uint32_t seq = slot.seq.load(std::memory_order_acquire);
            int32_t diff = (int32_t)seq - (int32_t)head;

            if (diff == 0) 
            {
                // CAS (Compare And Swap)
                if (m_head.compare_exchange_strong(head, head + 1, std::memory_order_relaxed, std::memory_order_relaxed)) 
                {
                    slot.data = entry;
                    slot.seq.store(head + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) 
            {
                // queue full
                return false;
            }
            else 
            {
                // diff > 0: head is stale -> need to reload and re-execute
                head = m_head.load(std::memory_order_relaxed);
            }
        }
    }

    /**
     * @brief pop log message from MPSC queue (for consumer)
     *
     * @param Entry: output log message data
     * @return bool: if queue is empty, then it returns false. Otherwise, it returns true.
     */
    bool TryPop(Entry& out) noexcept 
    {
        uint32_t tail = m_tail.load(std::memory_order_relaxed);
        Slot &slot = m_slots[tail & (m_capacity - 1)];
        uint32_t seq  = slot.seq.load(std::memory_order_acquire);
        int32_t  diff = (int32_t)seq - (int32_t)(tail + 1);

        if (diff == 0) 
        {
            out = slot.data;
            m_tail.store(tail + 1, std::memory_order_relaxed);
            slot.seq.store(tail + m_capacity, std::memory_order_release);
            return true;
        }

        return false;
    }

    // only use for hint
    bool IsEmpty() const noexcept 
    {
        return m_head.load(std::memory_order_acquire) == m_tail.load(std::memory_order_acquire);
    }

    // Approximate size of queue (number of messages currently in queue)
    // This is an approximation due to concurrent access by multiple producers
    // Only use for monitoring/statistics, not for critical decisions
    size_t ApproxSize() const noexcept 
    {
        uint32_t h = m_head.load(std::memory_order_acquire);
        uint32_t t = m_tail.load(std::memory_order_acquire);
        // Wrapping subtraction handles uint32_t overflow correctly
        return static_cast<size_t>(h - t);
    }

    static constexpr size_t Capacity() noexcept { return m_capacity; }
    static constexpr size_t MsgLen() noexcept { return m_msgLen; }

private:
    struct Slot 
    {
        std::atomic<uint32_t> seq{0};   // queue number
        Entry                 data;
    };

    alignas(64) std::atomic<uint32_t> m_head{0};
    alignas(64) std::atomic<uint32_t> m_tail{0};
    alignas(64) std::array<Slot, m_capacity> m_slots;
};

#endif  // _DT_LOG_QUEUE_H_
