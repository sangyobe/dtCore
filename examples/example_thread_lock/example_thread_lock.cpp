#include "dtCore/src/dtThread/threadImp.h"
#include "dtCore/src/dtUtils/dtLock.hpp"
#include "dtCore/src/dtUtils/dtTerminal.h"
#include <atomic>
#include <iostream>

using namespace dt::Thread;
constexpr int num_thread = 20;
constexpr int inc_per_thread = 500;
// #define NO_LOCK 1
#ifdef NO_LOCK
typedef struct _DummyLock
{
    bool lock() { return true; }
    bool unlock() { return true;}
} DummyLock;
DummyLock lock;
#else 
dt::Utils::Lock lock;
#endif

void *threadProc(void *arg)
{
    int *sum = (int *)arg;
    for (int i = 0; i < inc_per_thread; i++)
    {
        while (!lock.lock()); // spin lock

        *sum = *sum + 1;
        lock.unlock();

        SleepForMillis(1);
    }
    return 0;
}

int main(int argc, char **argv)
{
    int sum = 0;
    ThreadInfo th_info[num_thread];

    for (int i = 0; i < num_thread; i++)
    {
        th_info[i].name = "sum";
        th_info[i].procFunc = threadProc;
        th_info[i].procFuncArg = (void*)&sum;
        th_info[i].priority = 0;
        th_info[i].cpuIdx = 0;
        th_info[i].stackSz = 64 * 1024;

        int th = CreateNonRtThread(th_info[i]);
    }

    DeleteAllThread(); // wait until all thread end

    dtTerm::Printf("Sum = %d (expected: %d)\n", sum, num_thread * inc_per_thread);

    return 0;
}
