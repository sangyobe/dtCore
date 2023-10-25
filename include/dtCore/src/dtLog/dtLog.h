// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTLOG_HPP__
#define __DTCORE_DTLOG_HPP__

/** \defgroup dtLog
 *
 */

#define SPDLOG_COMPILED_LIB 1
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>
#include <spdlog/details/os.h>
#include <sstream>
#include <string>

#define DTCORE_DTLOG_MT

namespace dtCore {

using log_clock = ::std::chrono::system_clock;

class dtLog 
{
private:
    static std::string annotate_filename_datetime(const std::string file_basename)
    {
        spdlog::filename_t basename, ext, filename;

        time_t tnow = log_clock::to_time_t(log_clock::now());
        tm now_tm = spdlog::details::os::localtime(tnow);
        
        std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(file_basename);

        filename = fmt::format(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}{}"), basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1,
            now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ext);

        return filename;
    }

public:
    static void Initialize(const std::string log_name, const std::string file_basename = "") 
    {
        std::shared_ptr<spdlog::logger> logger{nullptr};
        // Create a logger
        if (file_basename.empty()) {
#ifdef DTCORE_DTLOG_MT
            logger = spdlog::stdout_color_mt(log_name);
#else
            logger = spdlog::stdout_color_st(log_name);
#endif 
        }
        else {
            spdlog::filename_t filename = annotate_filename_datetime(file_basename);
#ifdef DTCORE_DTLOG_MT
            logger = spdlog::basic_logger_mt(log_name, filename);
#else
            logger = spdlog::basic_logger_st(log_name, filename);
#endif 
        }
        logger->set_pattern("%^[%L][%H:%M:%S.%f]%$%v");
        spdlog::set_default_logger(logger);
    }

    static void Create(const std::string log_name, const std::string file_basename)
    {
        spdlog::filename_t filename = annotate_filename_datetime(file_basename);

        // Create a logger
#ifdef DTCORE_DTLOG_MT
        auto logger = spdlog::basic_logger_mt(log_name, filename);
#else
        auto logger = spdlog::basic_logger_st(log_name, filename);
#endif
        logger->set_pattern("%^[%L][%H:%M:%S.%f]%$%v");
    }

    static void Terminate() 
    {
        // flush all peding log message
        spdlog::shutdown();
    }

    enum class LogLevel {
        trace = spdlog::level::trace,
        debug = spdlog::level::debug, 
        info = spdlog::level::info, 
        warn = spdlog::level::warn, 
        err = spdlog::level::err, 
        critical = spdlog::level::critical, 
        off = spdlog::level::off
    };
    
    static void SetLogLevel(const std::string log_name, LogLevel lvl) {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        if (logger) {
            logger->set_level(static_cast<spdlog::level::level_enum>(lvl));
        }
    }

    static void SetLogLevel(LogLevel lvl) {
        spdlog::set_level(static_cast<spdlog::level::level_enum>(lvl));
    }

    static void SetLogOff() {
        spdlog::set_level(spdlog::level::off);
    }

    typedef uint32_t LogPattern;
    
    class LogPatternFlag {
    public:
        enum _flag {
            none         = 0,
            type         = 0x0001,
            type_long    = 0x0002,
            date         = 0x0010,
            time         = 0x0020,
            datetime     = 0x0040,
            epoch        = 0x0080,
            elapsed      = 0x0100,
            name         = 0x0004,
        };
    };

    static void SetLogPattern(const std::string log_name, LogPattern ptn = LogPatternFlag::type|LogPatternFlag::time , const std::string delimitter = "|") {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        std::string pattern = "%^";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimitter : 
                   (ptn & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimitter : "";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimitter :
                   (ptn & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimitter :
                   (ptn & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimitter : 
                   (ptn & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimitter : "";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimitter : "";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimitter : "";
        pattern += "%$%v";
        if (logger) {
            logger->set_pattern(pattern);
        }
    }

    static void SetLogPattern(LogPattern ptn = LogPatternFlag::type|LogPatternFlag::time, const std::string delimitter = "|") {
        std::string pattern = "%^";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimitter : 
                   (ptn & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimitter : "";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimitter :
                   (ptn & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimitter :
                   (ptn & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimitter : 
                   (ptn & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimitter : "";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimitter : "";
        pattern += (ptn & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimitter : "";
        pattern += "%$%v";
        spdlog::set_pattern(pattern);
    }

public:
    class LogStream {
    public:
        explicit LogStream(const spdlog::level::level_enum log_level)
            : _log_level(log_level) {
        }
        template <typename T> LogStream& operator<<(const T& value);
        template<typename... Args> inline void format(spdlog::format_string_t<Args...> fmt_string, Args &&...args);
        ~LogStream() {
            spdlog::log(_log_level, "{}", _log_stream.str());
        }
    private:
        spdlog::level::level_enum _log_level;
        std::ostringstream _log_stream;
    };

    class NamedLogStream {
    public:
        explicit NamedLogStream(const std::string log_name, const spdlog::level::level_enum log_level)
            : _log_name(log_name), _log_level(log_level) {
        }
        template <typename T> NamedLogStream& operator<<(const T& value);
        template<typename... Args> inline void format(spdlog::format_string_t<Args...> fmt_string, Args &&...args);
        ~NamedLogStream() {
            std::shared_ptr<spdlog::logger> logger = spdlog::get(_log_name);
            if (logger)
                logger->log(_log_level, "{}", _log_stream.str());
            else
                spdlog::log(_log_level, "{}", _log_stream.str());
        }
    private:
        std::string _log_name{};
        spdlog::level::level_enum _log_level;
        std::ostringstream _log_stream;
    };

};

} // namespace dtCore

/* Might cause macro redefinition errors, you can comment them out if needed */
// #define INFO info
// #define WARNING warn
// #define ERROR err

// dtLog to spdlog converter
#define LOG_S(log_level) dtCore::dtLog::LogStream(spdlog::level::log_level)
#define LOG_U_S(log_name, log_level) dtCore::dtLog::NamedLogStream(#log_name, spdlog::level::log_level)

// log to default logger
#define LOG(log_level) LOG_S(log_level)
// log to user-added log file
#define LOG_U(log_name, log_level) LOG_U_S(log_name, log_level)

#include "dtLog.tpp"

#endif // __DTCORE_DTLOG_HPP__