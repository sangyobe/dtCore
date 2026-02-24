#include <dtCore/src/dtLog/dtLog.h>
#include <thread>

void DEMO_log_str()
{
    LOG(trace) << "Hello, dtLog.";
    LOG(debug) << "Hello, dtLog.";
    LOG(info) << "Hello, dtLog.";
    LOG(warn) << "Hello, dtLog.";
    LOG(err) << "Hello, dtLog.";
    LOG(critical) << "Hello, dtLog.";
}

int main(int argc, const char **argv)
{
    dt::Log::Initialize(argv[0], "_SYSLOG_");
    
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    DEMO_log_str();

    dt::Log::Terminate();
    return 0;
}