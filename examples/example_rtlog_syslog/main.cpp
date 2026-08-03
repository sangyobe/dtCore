#include <dtCore/dtLog>
#include <thread>
#include <unistd.h>
#include <fcntl.h>

const uint32_t EXECUTION_TIME = 50*1000;     // 50ms
const uint32_t RELEASE_TIME = 50*1000;      // 50ms

void DEMO_log_str(bool _syslog = false)
{
    LOG(trace) << (_syslog ? "(syslog)" : "") << "Hello, dtLog.";
    LOG(debug) << (_syslog ? "(syslog)" : "") << "Hello, dtLog.";
    LOG(info) << (_syslog ? "(syslog)" : "") << "Hello, dtLog.";
    LOG(warn) << (_syslog ? "(syslog)" : "") << "Hello, dtLog.";
    LOG(err) << (_syslog ? "(syslog)" : "") << "Hello, dtLog.";
    LOG(critical) << (_syslog ? "(syslog)" : "") << "Hello, dtLog.";
}

int main(int argc, const char **argv)
{
    // Test#1. only syslog
    LOG_RT_RAW(info, "Test#1. only syslog");
    dt::Log::Initialize(argv[0], "_SYSLOG_", true);
    
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    DEMO_log_str(true);

    usleep(EXECUTION_TIME);   // EXECUTION_TIME delay

    dt::Log::Terminate();
    usleep(RELEASE_TIME);   // RELEASE_TIME delay

    // Test#2. only stdout
    LOG_RT_RAW(info, "Test#2. only stdout");
    dt::Log::Initialize(argv[0], "_STDOUT_");
    
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    DEMO_log_str(false);

    usleep(EXECUTION_TIME);   // EXECUTION_TIME delay

    dt::Log::Terminate();
    usleep(RELEASE_TIME);   // RELEASE_TIME delay

    // Test#3. TUI auto-disable when stdout is not a terminal
    // Simulate daemon/non-terminal environment by redirecting stdout to /dev/null.
    // isatty(STDOUT_FILENO) returns 0 → Initialize() auto-disables TUI and
    // prints a warning to STDERR via LogRaw even though enableTui=true was requested.
    LOG_RT_RAW(info, "Test#3. TUI auto-disable when stdout is not a terminal");
    {
        int saved_stdout = dup(STDOUT_FILENO);
        int devnull = open("/dev/null", O_WRONLY);
        dup2(devnull, STDOUT_FILENO);
        close(devnull);

        dt::Log::Initialize(argv[0], "", true);  // enableTui=true but should be suppressed

        dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
        DEMO_log_str();

        usleep(EXECUTION_TIME);   // EXECUTION_TIME delay

        dt::Log::Terminate();
        usleep(RELEASE_TIME);   // RELEASE_TIME delay

        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
    }
    LOG_RT_RAW(info, "Test#3. done: TUI was auto-disabled (warning above should appear on stderr)");
    return 0;
}
