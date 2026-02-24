// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_LOG_H__
#define __DT_LOG_H__

/** \defgroup dtLog
 *
 */

//#define SPDLOG_COMPILED_LIB 1
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/syslog_sink.h>
#include <spdlog/spdlog.h>
#include <spdlog/details/os.h>
#include <sstream>
#include <string>
#if defined(_WIN32) || defined(__CYGWIN__)
#else
#include <unistd.h>
#endif

#define DT_LOG_MT

namespace dt
{

using log_clock = ::std::chrono::system_clock;

class Log
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

    static std::tuple<std::string, std::string> split_by_directory(const std::string &fname)
    {
        auto dir_index = fname.rfind('/');

        // no valid directory found - return empty string as folder and whole path
        if (dir_index == std::string::npos)
        {
            return std::make_tuple(std::string(), fname);
        }
        // ends up with '/' - return whole path as directory and empty string as filename
        else if (dir_index == fname.size() - 1)
        {
            return std::make_tuple(fname, std::string());
        }

        // finally - return a valid directory and file path tuple
        return std::make_tuple(fname.substr(0, dir_index+1), fname.substr(dir_index+1)); // '/' is included as directory name
    }

public:
    enum class LogLevel {
        trace = spdlog::level::trace,
        debug = spdlog::level::debug, 
        info = spdlog::level::info, 
        warn = spdlog::level::warn, 
        err = spdlog::level::err, 
        critical = spdlog::level::critical, 
        off = spdlog::level::off
    };

    /**
     * Default logger를 생성하고 logging 시스템 초기화.
     * @param log_name default logger 이름.
     * @param file_basename 로그 파일 이름. 빈문자열 혹은 "_STDOUT_"인 경우 terminal. "_SYSLOG_" 는 posix 계열 OS에서 syslog. 그 외는 해당 파일명으로 로그 생성.
     * @param annot_datetime 파일 로그의 경우 파일 이름에 생성 날짜 및 시간을 뒤에 붙일지 여부.
     * @param truncate 파일 로그의 경우 동일 이름의 로그 파일이 있는 경우 해당 파일을 지우고 새로 만들지 여부. 만약 truncate 가 false(default)이면 해당 파일에 덛붙여 로그 기록.
     */
    static void Initialize(const std::string log_name, const std::string file_basename = "", bool annot_datetime = true, bool truncate = false) 
    {
        int rtn;
        std::shared_ptr<spdlog::logger> logger{nullptr};
        // Create a logger
        if (file_basename.empty() || file_basename == "_STDOUT_") {
#ifdef DT_LOG_MT
            logger = spdlog::stdout_color_mt(log_name);
#else
            logger = spdlog::stdout_color_st(log_name);
#endif 
            logger->set_pattern("%^[%L][%H:%M:%S.%f]%$%v");
        }
        else if (file_basename == "_SYSLOG_") {
#ifdef DT_LOG_MT
            logger = spdlog::syslog_logger_mt(log_name, log_name, LOG_PID, LOG_USER, true);
#else
            logger = spdlog::syslog_logger_st(log_name, log_name, LOG_PID, LOG_USER, true);
#endif
            logger->set_pattern("[%L]%v");
        }
        else {
            spdlog::filename_t filename = file_basename;
            if (annot_datetime)
                filename = annotate_filename_datetime(file_basename);
#ifdef DT_LOG_MT
            logger = spdlog::basic_logger_mt(log_name, filename, truncate);
#else
            logger = spdlog::basic_logger_st(log_name, filename, truncate);
#endif
            if (annot_datetime) {
                std::string dname, fname;
                std::tie(dname, fname) = split_by_directory(filename);
                rtn = remove(file_basename.c_str());
                // if (0 > rtn) logger->log(spdlog::level::warn, "{} Cannot remove previous symlink.", rtn);
#if defined(_WIN32) || defined(__CYGWIN__)
#else
                rtn = symlink(fname.c_str(), file_basename.c_str());
                if (0 > rtn) logger->log(spdlog::level::warn, "{} Cannot create symlink to this log file.", rtn);
#endif
            }
            logger->set_pattern("%^[%L][%H:%M:%S.%f]%$%v");
        }
        spdlog::set_default_logger(logger);
    }

    /**
     * Default logger외에 새로운 로거를 생성하고 등록.
     * @param log_name logger 이름.
     * @param file_basename 로그 파일 이름. 빈문자열 혹은 "_STDOUT_"인 경우 terminal. "_SYSLOG_" 는 posix 계열 OS에서 syslog. 그 외는 해당 파일명으로 로그 생성.
     * @param annot_datetime 파일 로그의 경우 파일 이름에 생성 날짜 및 시간을 뒤에 붙일지 여부.
     * @param truncate 파일 로그의 경우 동일 이름의 로그 파일이 있는 경우 해당 파일을 지우고 새로 만들지 여부. 만약 truncate 가 false(default)이면 해당 파일에 덛붙여 로그 기록.
     */
    static void Create(const std::string log_name, const std::string file_basename, bool annot_datetime = true, bool truncate = false)
    {
        int rtn;
        std::shared_ptr<spdlog::logger> logger{nullptr};

        if (file_basename == "_STDOUT_") {
#ifdef DT_LOG_MT
            logger = spdlog::stdout_color_mt(log_name);
#else
            logger = spdlog::stdout_color_st(log_name);
#endif 
            logger->set_pattern("%^[%L][%H:%M:%S.%f]%$%v");
        }
        else if (file_basename == "_SYSLOG_") {
#ifdef DT_LOG_MT
            logger = spdlog::syslog_logger_mt(log_name, log_name, LOG_PID, LOG_USER, true);
#else
            logger = spdlog::syslog_logger_st(log_name, log_name, LOG_PID, LOG_USER, true);
#endif 
            logger->set_pattern("[%L]%v");
        }
        else {
            spdlog::filename_t filename = file_basename;
            if (annot_datetime)
                filename = annotate_filename_datetime(file_basename);

            // Create a logger
#ifdef DT_LOG_MT
            logger = spdlog::basic_logger_mt(log_name, filename, truncate);
#else
            logger = spdlog::basic_logger_st(log_name, filename, truncate);
#endif
            if (annot_datetime) {
                std::string dname, fname;
                std::tie(dname, fname) = split_by_directory(filename);
                rtn = remove(file_basename.c_str());
                // if (0 > rtn) logger->log(spdlog::level::warn, "{} Cannot remove previous symlink.", rtn);
#if defined(_WIN32) || defined(__CYGWIN__)
#else
                rtn = symlink(fname.c_str(), file_basename.c_str());
                if (0 > rtn) logger->log(spdlog::level::warn, "{} Cannot create symlink to this log file.", rtn);
#endif
            }
            logger->set_pattern("%^[%L][%H:%M:%S.%f]%$%v");
        }
        // spdlog::register_logger(logger); // all loggers create by spdlog::create<Sink>() or spdlog::..._mt() functions are registered automatically.
    }

    /**
     * Logging 시스템 종료.
     */
    static void Terminate() 
    {
        // flush all peding log message
        spdlog::shutdown();
    }

    /**
     * 모든 등록된 logger 데이터를 주기적으로 flush 하도록 설정. Multi-thread 인 경우만 지원.
     * @param interval flush 간격(초).
     */
    static void FlushEvery(std::chrono::seconds interval)
    {
#ifdef DT_LOG_MT
        spdlog::flush_every(interval);
#endif
    }

    /**
     * Default logger off.
     */
    static void SetLogOff() {
        spdlog::default_logger()->set_level(spdlog::level::off);
    }

    /**
     * 해당 이름의 logger off
     */
    static void SetLogOff(const std::string log_name) {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        if (logger)
            logger->set_level(spdlog::level::off);
    }

    /**
     * 모든 log off.
     */
    static void SetLogOffAll() {
        spdlog::set_level(spdlog::level::off);
    }

    /**
     * Default logger 데이터를 flush(파일에 write)
     */
    static void Flush()
    {
        spdlog::default_logger()->flush();
    }

    /**
     * 해당 이름의 logger 데이터를 flush(파일에 write)
     * @param log_name logger 이름.
     */
    static void Flush(const std::string log_name)
    {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        if (logger)
            logger->flush();
    }

    /**
     * Default logger 데이터 flush 시점 설정.
     * @param lvl 해당 level의 로그가 기록될 때 flush.
     */
    static void FlushOn(LogLevel lvl)
    {
        spdlog::default_logger()->flush_on(static_cast<spdlog::level::level_enum>(lvl));
    }

    /**
     * 해당 이름의 logger 데이터 flush 시점 설정.
     * @param log_name logger 이름.
     * @param lvl 해당 level의 로그가 기록될 때 flush.
     */
    static void FlushOn(const std::string log_name, LogLevel lvl)
    {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        if (logger)
            logger->flush_on(static_cast<spdlog::level::level_enum>(lvl));
    }

    /**
     * 모든 logger 데이터 flush 시점 설정.
     * @param lvl 해당 level의 로그가 기록될 때 flush.
     */
    static void FlushOnAll(LogLevel lvl)
    {
        spdlog::flush_on(static_cast<spdlog::level::level_enum>(lvl));
    }

    /**
     * Default logger 의 log level 설정.
     * @param log_name logger 이름.
     * @param lvl log level.
     */
    static void SetLogLevel(LogLevel lvl) {
        spdlog::default_logger()->set_level(static_cast<spdlog::level::level_enum>(lvl));
    }
    
    /**
     * 해당 이름의 logger 의 log level 설정.
     * @param log_name logger 이름.
     * @param lvl log level.
     */
    static void SetLogLevel(const std::string log_name, LogLevel lvl) {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        if (logger) {
            logger->set_level(static_cast<spdlog::level::level_enum>(lvl));
        }
    }

    /**
     * 모든 logger 의 log level 설정.
     * @param lvl log level.
     */
    static void SetLogLevelAll(LogLevel lvl) {
        spdlog::set_level(static_cast<spdlog::level::level_enum>(lvl));
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

    /**
     * Default logger 로그 패턴 설정(spdlog string)
     * @param pattern 로그 패턴(spdlog string)
     */
    static void SetLogPattern(LogPattern pattern = LogPatternFlag::type|LogPatternFlag::time, const std::string delimitter = "|") {
        std::string pattern_str = "%^";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimitter : 
                       (pattern & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimitter :
                       (pattern & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimitter :
                       (pattern & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimitter : 
                       (pattern & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimitter : "";
        pattern_str += "%$%v";
        spdlog::default_logger()->set_pattern(pattern_str);
    }

    /**
     * 해당 이름의 logger 로그 패턴 설정(spdlog string)
     * @param log_name logger 이름.
     * @param pattern 로그 패턴(spdlog string)
     */
    static void SetLogPattern(const std::string log_name, LogPattern pattern, const std::string delimitter = "|") {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        std::string pattern_str = "%^";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimitter : 
                       (pattern & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimitter :
                       (pattern & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimitter :
                       (pattern & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimitter : 
                       (pattern & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimitter : "";
        pattern_str += "%$%v";
        if (logger) {
            logger->set_pattern(pattern_str);
        }
    }

    /**
     * 모든 logger 로그 패턴 설정(spdlog string)
     * @param pattern 로그 패턴(spdlog string)
     */
    static void SetLogPatternAll(LogPattern pattern = LogPatternFlag::type|LogPatternFlag::time, const std::string delimitter = "|") {
        std::string pattern_str = "%^";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::type))      ? std::string("%L") + delimitter : 
                       (pattern & static_cast<LogPattern>(LogPatternFlag::type_long)) ? std::string("%l") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::date))      ? std::string("%Y-%m-%d") + delimitter :
                       (pattern & static_cast<LogPattern>(LogPatternFlag::time))      ? std::string("%H:%M:%S.%f") + delimitter :
                       (pattern & static_cast<LogPattern>(LogPatternFlag::datetime))  ? std::string("%Y-%m-%d %H:%M:%S.%f") + delimitter : 
                       (pattern & static_cast<LogPattern>(LogPatternFlag::epoch))     ? std::string("%E.%f") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::elapsed))   ? std::string("%8i") + delimitter : "";
        pattern_str += (pattern & static_cast<LogPattern>(LogPatternFlag::name))      ? std::string("%n") + delimitter : "";
        pattern_str += "%$%v";
        spdlog::set_pattern(pattern_str);
    }

    /**
     * Default logger 로그 패턴 설정(spdlog string)
     * @param patter 로그 패턴(spdlog string)
     */
    static void SetLogPattern(const std::string pattern) {
        // spdlog::set_pattern(pattern);
        spdlog::default_logger()->set_pattern(pattern);
    }

    /**
     * 해당 이름의 logger 로그 패턴 설정(spdlog string)
     * @param log_name logger 이름.
     * @param patter 로그 패턴(spdlog string)
     */
    static void SetLogPattern(const std::string log_name, const std::string pattern) {
        std::shared_ptr<spdlog::logger> logger = spdlog::get(log_name);
        if (logger) {
            logger->set_pattern(pattern);
        }
    }

    /**
     * 모든 logger 로그 패턴 설정(spdlog string)
     * @param patter 로그 패턴(spdlog string)
     */
    static void SetLogPatternAll(const std::string pattern) {
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

} // namespace dt

/* Might cause macro redefinition errors, you can comment them out if needed */
// #define INFO info
// #define WARNING warn
// #define ERROR err

// dtLog to spdlog converter
#define LOG_S(log_level) dt::Log::LogStream(spdlog::level::log_level)
#define LOG_U_S(log_name, log_level) dt::Log::NamedLogStream(#log_name, spdlog::level::log_level)

// log to default logger
#define LOG(log_level) LOG_S(log_level)
// log to user-added log file
#define LOG_U(log_name, log_level) LOG_U_S(log_name, log_level)

#include "dtLog.tpp"

#endif // __DT_LOG_H__