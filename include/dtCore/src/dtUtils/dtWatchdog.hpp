// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_UTILS_WATCHDOG_H__
#define __DT_UTILS_WATCHDOG_H__

/** \addtogroup dtUtils
 *
 */
namespace dt
{
namespace Utils
{
template <int COUNTER_MAX>
class Watchdog
{
public:
    /**
     * Default constructor.
     * Watchdog counter를 최대값으로 초기화한다.
     */
    Watchdog()
    {
        _counter = COUNTER_MAX;
    }

    /**
     * Constructor.
     * @param counter_init counter의 초기값. 0보다 작으면 counter의 최대값으로 초기화한다. 
     */
    Watchdog(int counter_init)
    {
        if (counter_init < 0)
            _counter = COUNTER_MAX;
        else
            _counter = counter_init;
    }

    /**
     * Destructor
     */
    ~Watchdog() = default;

public:
    /**
     * 모니터링 하는 데이터의 유효성 여부를 체크하여 watchdog counter를 갱신한다.
     * watch flag 가 true 이면(새로운 데이터를 본 경우), counter를 리셋한다.
     * 그렇지 않은 경우는 counter를 감소(>0)시킨다.
     */
    void Watch(bool watch = false)
    {
        if (watch)
            _counter = COUNTER_MAX;
        else if (_counter > 0)
            _counter--;
    }

    /**
     * Reset counter with value
     * @param counter value to reset counter as.
     */
    void Reset(int counter)
    {
        _counter = counter;
    }

    /**
     * 모니터링하는 데이터의 유효성 여부를 리턴한다.
     * @return true if it is up-to-date.
     */
    bool IsValid()
    {
        return (_counter > 0);
    }

private:
    int _counter;
};

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_WATCHDOG_H__