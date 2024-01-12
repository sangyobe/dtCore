// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASINKPBHDF5_H__
#define __DTCORE_DTDATASINKPBHDF5_H__

/** \defgroup dtDAQ
 *
 */
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <deque>

#include "dtDataSinkPB.hpp"

namespace dtCore {

    using log_clock = ::std::chrono::system_clock;

template<typename StateType>
class dtDataSinkPBHdf5 : public dtDataSinkPB<StateType>
{
private:
    // static std::string annotate_filename_datetime(const std::string file_basename)
    // {
    //     spdlog::filename_t basename, ext, filename;

    //     time_t tnow = log_clock::to_time_t(log_clock::now());
    //     tm now_tm = spdlog::details::os::localtime(tnow);
        
    //     std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(file_basename);

    //     filename = fmt::format(SPDLOG_FILENAME_T("{}_{:04d}-{:02d}-{:02d}_{:02d}-{:02d}-{:02d}{}"), basename, now_tm.tm_year + 1900, now_tm.tm_mon + 1,
    //         now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ext);

    //     return filename;
    // }

    // static std::tuple<std::string, std::string> split_by_directory(const std::string &fname)
    // {
    //     auto dir_index = fname.rfind('/');

    //     // no valid directory found - return empty string as folder and whole path
    //     if (dir_index == std::string::npos)
    //     {
    //         return std::make_tuple(std::string(), fname);
    //     }
    //     // ends up with '/' - return whole path as directory and empty string as filename
    //     else if (dir_index == fname.size() - 1)
    //     {
    //         return std::make_tuple(fname, std::string());
    //     }

    //     // finally - return a valid directory and file path tuple
    //     return std::make_tuple(fname.substr(0, dir_index+1), fname.substr(dir_index+1)); // '/' is included as directory name
    // }
    
public:
    dtDataSinkPBHdf5(const std::string file_basename = "", bool annot_datetime = true) {}
    ~dtDataSinkPBHdf5() {}

    void Publish(StateType& msg) {}
};

#endif // __DTCORE_DTDATASINKPBHDF5_H__