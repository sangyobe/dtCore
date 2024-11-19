#include <dtCore/src/dtThread/threadImp.h> // for SleepForMillies()
#include <dtCore/src/dtUtils/dtTimeCheck.h>
#include <iostream>

int main(int argc, const char **argv)
{
    dt::Utils::TimeCheck timer;
    timer.Start();
    {
        // Do some time consuming job...
        dt::Thread::SleepForMillis(500);
    }
    timer.Stop();
    std::cout << "Elapsed time: " << timer.GetElapsedTime_msec() << " (ms)" << std::endl;
    return 0;
}