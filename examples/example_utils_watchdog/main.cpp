#include <dtCore/src/dtUtils/dtWatchdog.hpp>
#include <dtCore/src/dtThread/threadImp.h> // for SleepForMillies()
#include <dtCore/src/dtUtils/dtTerminal.h>
#include <atomic>
#include <iostream>

dt::Utils::Watchdog<10> watchdog;
std::atomic<bool> bRun = false;

void *threadProc(void *)
{
    while (watchdog.IsValid())
    {
        dt::Thread::SleepForMillis(1000);
        watchdog.Watch(false);
        dtTerm::Printf("Dec watchdog counter.\n");
    }
    dtTerm::Printf("[ALARM]] Watchdog fired.\n");
    bRun.store(false);
    return 0;
}

int main(int argc, const char **argv)
{
    dt::Thread::ThreadInfo th_info;
    th_info.name = "My Thread";
    th_info.procFunc = threadProc;
    th_info.procFuncArg = (void*)&th_info;
    th_info.priority = 0;
    th_info.cpuIdx = 1;
    th_info.stackSz = 10 * 1024 * 1024;

    int th = dt::Thread::CreateNonRtThread(th_info);

    dtTerm::SetupTerminal(false);
    bRun.store(true);
    while (bRun.load())
    {
        if (dtTerm::kbhit())
        {
            switch (getchar())
            {
            case 'Q':
            case 'q':
                bRun.store(false);
                break;
            case 'R':
            case 'r':
                watchdog.Watch(true);
                dtTerm::Printf("[NEW Message] Watchdog reset.\n");
                break;
            }
        }
        else
        {
            dt::Thread::SleepForMillis(1);
        }
    }

    dt::Thread::DeleteThread(th_info);
    dtTerm::RestoreTerminal();
    return 0;
}