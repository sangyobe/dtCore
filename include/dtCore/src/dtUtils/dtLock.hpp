// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_UTILS_LOCK_H__
#define __DT_UTILS_LOCK_H__

#include <atomic>

/** \defgroup dtUtils
 *
 */
namespace dt
{
namespace Utils
{
/**
 * Lock implementation.
 */
class Lock
{
public:
    bool lock() {
        bool expected = false;
        return _lock.compare_exchange_strong(expected, true);
    }
    void unlock() {
        _lock.store(false);
    }
private:
    std::atomic_bool _lock{false};
};

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_LOCK_H__