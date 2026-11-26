#include <dtCore/src/dtUtils/dtWatchdog.hpp>
#include <dtCore/src/dtUtils/dtTerminal.h>
#include <iostream>

int main(int argc, const char **argv)
{

    dt::Utils::Watchdog<10> wd;

    int count = 1;
    while (count < 101)
    {
        dtTerm::Printf("count: %d", count);

        if (!wd.DecAndReset())
            dtTerm::Printf(" [skip]");

        dtTerm::Printf("\n");
        count++;
    }

   return 0;
}