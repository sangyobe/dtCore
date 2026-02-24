#include <dtCore/src/dtLog/dtLog.h>
#include <thread>

void DEMO_filelog(const char *logname)
{
    dt::Log::Initialize(logname, "logs/example_log_file.log");
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    LOG(trace) << "Hello, dtLog.";
    LOG(debug) << "Hello, dtLog.";
    LOG(info) << "Hello, dtLog.";
    LOG(warn) << "Hello, dtLog.";
    LOG(err) << "Hello, dtLog.";
    LOG(critical) << "Hello, dtLog.";
    dt::Log::Terminate();
}

void DEMO_filelog_mt(const char *logname)
{
    dt::Log::Initialize(logname, "logs/example_log_file_mt.log");
    LOG(info) << "Program started.";

    std::size_t msg_count = 10;
    std::size_t thread_count = 10;
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (size_t t = 0; t < thread_count; ++t)
    {
        threads.emplace_back([ t, msg_count, thread_count ]() constexpr {
            for (int j = 0; j < msg_count; j++)
            {
                LOG(info).format("Hello dtLog! thread_number:msg_number {}:{}", t, j);
            }
        });
    }

    for (auto &t : threads)
    {
        t.join();
    };

    LOG(info) << "Program ended.";
    dt::Log::Terminate();
}

int main(int argc, const char **argv)
{
    DEMO_filelog(argv[0]);
    DEMO_filelog_mt(argv[0]);
    return 0;
}