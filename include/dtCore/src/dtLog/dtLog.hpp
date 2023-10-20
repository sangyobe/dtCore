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
#include <spdlog/spdlog.h>
#include <spdlog/details/os.h>
#include <sstream>
#include <string>

namespace dtCore {

using log_clock = ::std::chrono::system_clock;

class dtLog 
{
public:
    static void Initialize(std::string prog_name, std::string file_basename) 
    {
        spdlog::filename_t basename, ext, filename;


        time_t tnow = log_clock::to_time_t(log_clock::now());
        tm now_tm = spdlog::details::os::localtime(tnow);
        
        std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(file_basename);

        filename = fmt::format(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}{}"), basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1,
            now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ext);

        // Create a logger
        auto logger = spdlog::basic_logger_mt(prog_name, filename);
        spdlog::set_default_logger(logger);
    }

    static void Terminate() 
    {
        // flush all peding log message
        spdlog::shutdown();
    }

public:
    class InputStream {
    public:
        explicit InputStream(const spdlog::level::level_enum log_level)
            : _log_level(log_level) {
        }
        template <typename T>
        InputStream& operator<<(const T& value) {
            _log_stream << value;
            return *this;
        }
        template<typename... Args>
        inline void format(spdlog::format_string_t<Args...> fmt_string, Args &&...args)
        {
            _log_stream << fmt::format(fmt_string, std::forward<Args>(args)...);
        }
        ~InputStream() {
            spdlog::log(_log_level, "{}", _log_stream.str());
        }
    private:
        spdlog::level::level_enum _log_level;
        std::ostringstream _log_stream;
    };

};

} // namespace dtCore

/* Might cause macro redefinition errors, you can comment them out if needed */
// #define INFO info
// #define WARNING warn
// #define ERROR err

// loguru
#define LOG_S(log_level) dtCore::dtLog::InputStream(spdlog::level::log_level)

// glog
#define LOG(log_level) LOG_S(log_level)


#endif // __DTCORE_DTLOG_HPP__