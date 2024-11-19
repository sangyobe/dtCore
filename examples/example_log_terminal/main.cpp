#include <dtCore/src/dtLog/dtLog.h>
#include <thread>

int main(int argc, const char **argv)
{
    dt::Log::Initialize(argv[0]);
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    LOG(info) << "Program started.";

    std::size_t msg_count = 100;
    std::size_t thread_count = 10;
    std::vector<std::thread> threads;
    threads.reserve(thread_count);
    for (size_t t = 0; t < thread_count; ++t)
    {
        threads.emplace_back([ t, msg_count, thread_count ]() constexpr {
            for (int j = 0; j < msg_count / static_cast<int>(thread_count); j++)
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
    return 0;
}