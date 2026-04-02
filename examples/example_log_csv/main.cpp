#include <dtCore/src/dtLog/dtLog.h>
#include <thread>

std::function<double(void)> data_gen = ([]() -> double { return (double)rand() / (double)RAND_MAX; });

int main(int argc, const char **argv)
{
    dt::Log::Initialize(argv[0]);

    // create a new csv log file
    dt::Log::Create("csv_file_1", "logs/example_log_csv.csv", false, true);
    dt::Log::SetLogLevel("csv_file_1", dt::Log::LogLevel::trace);
    // dt::Log::SetLogPattern("csv_file_1", dt::Log::LogPatternFlag::none, ""); // remove all log prefix (it is useful while logging as CSV-style)
    dt::Log::SetLogPattern("csv_file_1", dt::Log::LogPatternFlag::datetime, ","); // add comma seperated datetime

    // write CSV header
    LOG_U(csv_file_1, trace).format("val1,val2,val3");

    // write some data
    for (int i = 100; i > 0; i--)
        LOG_U(csv_file_1, trace).format("{},{},{}", data_gen(), data_gen(), data_gen());

    dt::Log::Terminate();
    return 0;
}