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
        
        if (counter > 10)
            break;
    }
    return 0;
}

int main(int argc, char **argv)
{
    dtTerm::SetupTerminal(false);

    bRun.store(true);

    ThreadInfo th_info;
    th_info.name = "My Thread";
    th_info.procFunc = threadProc;
    th_info.procFuncArg = (void*)&th_info;
    th_info.priority = 0;
    th_info.cpuIdx = 1;
    th_info.stackSz = 10 * 1024 * 1024;

    int th = CreateNonRtThread(th_info);

    DeleteThread(th_info);

    dtTerm::RestoreTerminal();
    return 0;
}