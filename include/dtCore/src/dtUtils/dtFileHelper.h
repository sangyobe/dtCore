// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

/*!
 \file      dtFileHelper.h
 \brief     File name manipulation helper functions.
 \author    Sangyup Yi, sean.yi@hyundai.com
 \date      2024. 04. 23
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef __DT_UTILS_FILEHELPER_H__
#define __DT_UTILS_FILEHELPER_H__

/** \defgroup dtUtils
 *
 */

#include <string>
#include <tuple>

namespace dt
{
namespace Utils
{

/**
 * check if path exists.
 */
bool path_exists(const std::string &filename);

/**
 * make a directory.
 */
bool mkdir_(const std::string &path);

/**
 * split file path into directory and file name.
 */
std::tuple<std::string, std::string> split_by_directory(const std::string &fname);

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
std::tuple<std::string, std::string> split_by_extension(const std::string &fname);

/**
 * append datetime to given filename.
 */
std::string annotate_filename_datetime(const std::string file_basename);

/**
 * append index(number) to given filename.
 */
std::string annotate_filename_index(const std::string file_basename, std::size_t index);

/**
 * create the given directory - and all directories leading to it.
 * @return true on success or if the directory already exists.
 */
bool create_dir(const std::string &path);

/**
 * delete the target if exists, and rename the src file to target.
 * return true on success, false otherwise..
 */
bool rename_file(const std::string &src_filename, const std::string &target_filename) noexcept;

/**
 * get file size.
 * @return file size according to open FILE* object.
 */
std::size_t filesize(FILE *f);

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_FILEHELPER_H__