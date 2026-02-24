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
    dt::Log::Initialize(argv[0]);
    
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    DEMO_log_str();

    // set custom log pattern(ORed flags)
    dt::Log::SetLogPattern(dt::Log::LogPatternFlag::type|dt::Log::LogPatternFlag::time, "|");
    DEMO_log_str();

    // set custom log pattern(spdlog pattern string)
    // Flag Description
    // %v   The actual log message
    // %t   Thread id
    // %P   Process id
    // %n   Logger name
    // %l   Log level (e.g., info, debug)
    // %L   Short log level (e.g., I, D)
    // %a   Abbreviated weekday name (e.g., Mon)	
    // %A   Full weekday name (e.g., Monday)	
    // %b   Abbreviated month name (e.g., Jan)	
    // %B   Full month name (e.g., January)	
    // %c   Date and time representation	
    // %C   Year without century (00-99)	
    // %d   Day of the month (01-31)	
    // %D   Short date format (MM/DD/YY)	
    // %e   Milliseconds part of the timestamp
    // %F   ISO 8601 date format (YYYY-MM-DD)	
    // %H   Hour in 24h format (00-23)	
    // %I   Hour in 12h format (01-12)	
    // %j   Day of the year (001-366)	
    // %m   Month as a decimal number (01-12)	
    // %M   Minute (00-59)	
    // %p   AM/PM designation	
    // %S   Second (00-59)	
    // %T   ISO 8601 time format (HH:MM:SS)
    // %Y   Year with century	
    // %z   Time zone name or abbreviation
    // %^   Start log level color (console log only)	
    // %$   End log level color (console log only)	
    // %s   Source file name (requires SPDLOG_INFO macros)
    // %!   Function name (requires SPDLOG_INFO macros)
    // %#   Line number (requires SPDLOG_INFO macros)
    // %@   Source file name and line number (requires SPDLOG_INFO macros)
    dt::Log::SetLogPattern("[%Y-%m-%d %H:%M:%S.%f][%^%l%$] %n: %v");
    DEMO_log_str();

    dt::Log::Terminate();
    return 0;
}