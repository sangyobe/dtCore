#include "dtCore/src/dtThread/threadImp.h"
#include "dtCore/src/dtUtils/dtTerminal.h"
#include <atomic>
#include <iostream>

using namespace dt::Thread;
static std::atomic<bool> bRun = false;

void *threadProc(void *arg)
{
    ThreadInfo* th_info = (ThreadInfo*)arg;
    int counter = 0;
    while (bRun.load())
    {
        dtTerm::Printf("[%s]counter : %d\n", th_info->name, counter++);

        dt::Thread::SleepForMillis(500);
        // for (int i = 0; i < 10000000; i++)
        //     ;
    }
    return 0;
}

int main(int argc, char **argv)
{
    dtTerm::SetupTerminal(false);

    bRun.store(true);

    ThreadInfo th1_info;
    th1_info.name = "My Thread - 1";
    th1_info.procFunc = threadProc;
    th1_info.procFuncArg = (void*)&th1_info;
    th1_info.priority = 0;
    th1_info.cpuIdx = 2;
    th1_info.stackSz = 10 * 1024 * 1024;

    int th1 = CreateNonRtThread(th1_info);

    ThreadInfo th2_info;
    th2_info.name = "My Thread - 2";
    th2_info.procFunc = threadProc;
    th2_info.procFuncArg = (void*)&th2_info;
    th2_info.priority = 0;
    th2_info.cpuIdx = 3;
    th2_info.stackSz = 10 * 1024 * 1024;

    int th2 = CreateNonRtThread(th2_info);

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
