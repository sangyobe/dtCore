#include "dtCore/src/dtThread/threadImp.h"
#include "dtCore/src/dtUtils/dtTerminal.h"
#include <atomic>
#include <iostream>

using namespace dt::Thread;
static std::atomic<bool> bRun = false;

void *threadProc(void *arg)
{
    int counter = 0;
    while (bRun.load())
    {
        for (int i = 0; i < 1000000; i++)
            ;
        //std::cout << "counter : " << counter++ << std::endl;

        // if (dtTerm::kbhit())
        // {
        //     getchar();
        //     break;
        // }
    }
    return 0;
}

int main(int argc, char **argv)
{
    int cpu = 3;
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpu, &mask);

    pid_t pid = getpid();

    if (sched_setaffinity(pid, sizeof(mask), &mask))
    {
        dtTerm::Print("Cannot set cpu affinity.");
    }

    dtTerm::SetupTerminal(false);

    bRun.store(true);

    ThreadInfo th_info;
    th_info.name = "My Thread";
    th_info.procFunc = threadProc;
    th_info.procFuncArg = nullptr;
    th_info.priority = 0;
    th_info.cpuIdx = 1;
    th_info.stackSz = 10 * 1024 * 1024;

    int th1 = CreateNonRtThread(th_info);

    th_info.name = "My Thread - 2";
    th_info.procFunc = threadProc;
    th_info.procFuncArg = nullptr;
    th_info.priority = 0;
    th_info.cpuIdx = 2;
    th_info.stackSz = 10 * 1024 * 1024;

    int th2 = CreateNonRtThread(th_info);

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
            }
        }
        else
        {
            SleepForMillis(1);
        }
    }

    DeleteAllThread();
    dtTerm::RestoreTerminal();

    return 0;
}
