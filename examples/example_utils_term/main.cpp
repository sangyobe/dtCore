#include <dtCore/src/dtThread/threadImp.h> // for SleepForMillies()
#include <dtCore/src/dtUtils/dtTerminal.h>
#include <dtCore/version.h>
#include <atomic>

typedef struct _DispData
{
    typedef struct _TaskState
    {
        std::string name;
        int status{0};
    } TaskState;
    TaskState taskState;

    std::atomic<bool> restartDisp{false};
} DispData;

void InitDisp();
void PrintTaskState(int r, DispData::TaskState *state);

std::atomic<bool> bRun = false;

void *procDisp(void *data)
{
    dt::Thread::SleepForMillis(1000);
    InitDisp();

    DispData *dispData = (DispData *)data;
    while (bRun.load())
    {
        if (dispData->restartDisp.load())
        {
            InitDisp();
            dispData->restartDisp.store(false);
        }
        PrintTaskState(2, &(dispData->taskState));
        dispData->taskState.status++;
        dt::Thread::SleepForMillis(10);
    }
    dtTerm::Print("\n\n");
    return 0;
}

int main(int argc, const char **argv)
{
    dtTerm::SetupTerminal(false); // show cursor - false
    dtTerm::ClearDisp();

    dtTerm::PrintTitle("ART Framework");
    dtTerm::Printf(FG_CYAN, "Version: " DTCORE_VERSION_STR "\n\n");
    dtTerm::PrintEndLine();
    dt::Thread::SleepForMillis(1000);
    
    DispData dispData;
    dispData.taskState.name = "TUI Demo";
    dispData.taskState.status = 0;
    dt::Thread::ThreadInfo th_info;
    th_info.name = "TUI Thread";
    th_info.procFunc = procDisp;
    th_info.procFuncArg = (void*)&dispData;
    th_info.priority = 0;
    th_info.cpuIdx = 1;
    th_info.stackSz = 1 * 1024 * 1024;

    int th = dt::Thread::CreateNonRtThread(th_info);

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
                dispData.restartDisp.store(true);
                dispData.taskState.status = 0;
                break;
            }
        }
        else
        {
            dt::Thread::SleepForMillis(1);
        }
    }
    dt::Thread::DeleteThread(th_info);

    dtTerm::ClearDisp();
    dtTerm::RestoreTerminal();
    return 0;
}

void InitDisp()
{
    dtTerm::ClearDisp();
    dtTerm::CurPos(1, 1);
    //                     00000000011111111112222222222333333333344444444445555555555666666666677777777778
    //                     12345678901234567890123456789012345678901234567890123456789012345678901234567890
    dtTerm::Printf(+1, 1, "***** Task State ***************************************************************");
    dtTerm::Printf(+2, 1, "  Name: (n/a)                                                                   ");
    dtTerm::Printf(+3, 1, "Status: (n/a)                                                                   ");
    dtTerm::Printf(+4, 1, "                                                                                ");
}

void PrintTaskState(int r, DispData::TaskState *state)
{
    dtTerm::Printf(r, +9, "%s", state->name.c_str());
    r++;
    dtTerm::Printf(r, +9, FG_RED, "%10d", state->status);
    r++;

    dtTerm::Flush();
}