// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

/*!
 \file      dtFileHelper.hpp
 \brief     File name manipulation helper functions.
 \author    Sangyup Yi, sean.yi@hyundai.com
 \date      2024. 04. 23
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef __DTCORE_DTFILEHELPER_H__
#define __DTCORE_DTFILEHELPER_H__

/** \defgroup dtUtils
 *
 */

#include <string>
#include <tuple>
#include <chrono>
#include <sys/stat.h>
#include "dtStringHelper.hpp"

#if !defined(DTCORE_FOLDER_SEPS)
    #ifdef _WIN32
        #define DTCORE_FOLDER_SEPS "\\/"
    #else
        #define DTCORE_FOLDER_SEPS "/"
    #endif
#endif

namespace dtCore {

bool path_exists(const std::string &filename)
{
#ifdef _WIN32
    auto attribs = ::GetFileAttributesA(filename.c_str());
    return attribs != INVALID_FILE_ATTRIBUTES;
#else // common linux/unix all have the stat system call
    struct stat buffer;
    return (::stat(filename.c_str(), &buffer) == 0);
#endif
}

bool mkdir_(const std::string &path) {
#ifdef _WIN32
    return ::_mkdir(path.c_str()) == 0;
#else
    return ::mkdir(path.c_str(), mode_t(0755)) == 0;
#endif
}

/**
 * split file path into directory and file name.
 */
using _clock = ::std::chrono::system_clock;

std::tuple<std::string, std::string> split_by_directory(const std::string &fname)
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

/**
 * split file path into file path and its file extension.
 * 
 * "file.txt" => ("file", ".txt")
 * "file" => ("file", "")
 * "file." => ("file.", "")
 * "/dir1/dir2/file.txt" => ("/dir1/dir2/file", ".txt")
 * 
 * the starting dot in filenames is ignored (hidden files):
 * ".file" => (".file". "")
 * "dir/.file" => ("dir/.file", "")
 * "dir/.file.txt" => ("dir/.file", ".txt")
 */
std::tuple<std::string, std::string> split_by_extension(const std::string &fname) 
{
    auto ext_index = fname.rfind('.');

    // no valid extension found - return whole path and empty string as extension
    if (ext_index == std::string::npos || ext_index == 0 || ext_index == fname.size() - 1) {
        return std::make_tuple(fname, std::string());
    }

    // treat cases like "/etc/rc.d/somelogfile or "/abc/.hiddenfile"
    auto folder_index = fname.find_last_of(DTCORE_FOLDER_SEPS);
    if (folder_index != std::string::npos && folder_index >= ext_index - 1) {
        return std::make_tuple(fname, std::string());
    }

    // finally - return a valid base and extension tuple
    return std::make_tuple(fname.substr(0, ext_index), fname.substr(ext_index));
}

std::tuple<std::string, std::string> split_by_extension(const std::string &fname);

/**
 * append datetime to given filename
 */
std::string annotate_filename_datetime(const std::string file_basename)
{
    std::string basename, ext, filename;

    time_t tnow = _clock::to_time_t(_clock::now());
    tm now_tm;
#ifdef _WIN32
    ::localtime_s(&now_tm, &tnow);
#else
    ::localtime_r(&tnow, &now_tm);
#endif
    
    std::tie(basename, ext) = split_by_extension(file_basename);

    filename = string_format("%s_%04d-%02d-%02d_%02d-%02d-%02d%s", basename.c_str(), now_tm.tm_year + 1900, now_tm.tm_mon + 1,
        now_tm.tm_mday, now_tm.tm_hour, now_tm.tm_min, now_tm.tm_sec, ext.c_str());

    return filename;
}

/**
 * create the given directory - and all directories leading to it
 * return true on success or if the directory already exists
 */
bool create_dir(const std::string &path) {
    if (path_exists(path)) {
        return true;
    }

    if (path.empty()) {
        return false;
    }

    size_t search_offset = 0;
    do {
        auto token_pos = path.find_first_of(DTCORE_FOLDER_SEPS, search_offset);
        // treat the entire path as a folder if no folder separator not found
        if (token_pos == std::string::npos) {
            token_pos = path.size();
        }

        auto subdir = path.substr(0, token_pos);

        if (!subdir.empty() && !path_exists(subdir) && !mkdir_(subdir)) {
            return false;  // return error if failed creating dir
        }
        search_offset = token_pos + 1;
    } while (search_offset < path.size());

    return true;
}

}

#endif // __DTCORE_DTFILEHELPER_H__