#include <dtCore/dtLog>
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

void DEMO_log_str_custom()
{
    LOG_U(logger_2, trace) << "Hello, dtLog.";
    LOG_U(logger_2, debug) << "Hello, dtLog.";
    LOG_U(logger_2, info) << "Hello, dtLog.";
    LOG_U(logger_2, warn) << "Hello, dtLog.";
    LOG_U(logger_2, err) << "Hello, dtLog.";
    LOG_U(logger_2, critical) << "Hello, dtLog.";
}

int main(int argc, const char **argv)
{
    dt::Log::Initialize("logger_1");
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    DEMO_log_str();

    dt::Log::Create("logger_2", "_STDOUT_");
    dt::Log::SetLogLevel("logger_2", dt::Log::LogLevel::info);
    dt::Log::SetLogPattern("logger_2", dt::Log::LogPatternFlag::type | dt::Log::LogPatternFlag::date, "|");
    DEMO_log_str_custom();

    auto thread_1 = std::thread([]() {
        for (int i = 0; i < 500; i++)
        {
            LOG(debug) << "logger_1: 1111111111111111111111111111111";
            usleep(10 * 1000);
        }
    });

    auto thread_2 = std::thread([]() {
        for (int i = 0; i < 500; i++)
        {
            LOG_U(logger_2, info) << "logger_2: 2222222222222222222222222222222";
            usleep(10 * 1000);
        }
    });

    thread_1.join();
    thread_2.join();
    
    dt::Log::Terminate();
    return 0;
}
