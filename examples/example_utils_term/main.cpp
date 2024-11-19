#include <dtCore/src/dtThread/threadImp.h> // for SleepForMillies()
#include <dtCore/src/dtUtils/dtTerminal.h>
#include <dtCore/version.h>

int main(int argc, const char **argv)
{
    dtTerm::SetupTerminal(false); // show cursor - false
    dtTerm::ClearDisp();

    dtTerm::PrintTitle("ART Framework");
    dtTerm::Printf(FG_CYAN, "Version: " DTCORE_VERSION_STR "\n\n");
    dtTerm::PrintEndLine();
    dt::Thread::SleepForMillis(1000);
    dtTerm::ClearDisp();

    dtTerm::Printf(1, 1, "(1,1) Hello");
    dtTerm::Printf(2, 2, "(2,2) Hello");
    dtTerm::Printf(3, 3, FG_RED, "(3,3) Hello");
    dtTerm::Printf(4, 4, FG_MAGENTA, "(4,4) Hello");
    dtTerm::Flush();
    dt::Thread::SleepForMillis(1000);

    dtTerm::ClearDisp();
    dtTerm::RestoreTerminal();
    return 0;
}