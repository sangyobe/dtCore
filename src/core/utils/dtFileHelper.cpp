//* Related Headers ----------------------------------------------------------*/
#include "dtCore/src/dtUtils/dtFileHelper.h"
#include "dtCore/src/dtUtils/dtStringHelper.h"

#include <string>
#include <tuple>
#include <chrono>
#include <sys/stat.h>

#if !defined(DTCORE_FOLDER_SEPS)
    #ifdef _WIN32
        #define DTCORE_FOLDER_SEPS "\\/"
    #else
        #define DTCORE_FOLDER_SEPS "/"
    #endif
#endif

namespace dt
{
namespace Utils
{

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
 * append index(number) to given filename
 */
std::string annotate_filename_index(const std::string file_basename, std::size_t index)
{
    if (index == 0u)
    {
        return file_basename;
    }

    std::string basename, ext, filename;
    std::tie(basename, ext) = split_by_extension(file_basename);

    filename = string_format("%s.%d%s", basename.c_str(), index, ext.c_str());

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

/**
 * delete the target if exists, and rename the src file to target
 * return true on success, false otherwise.
 */
bool rename_file(const std::string &src_filename, const std::string &target_filename) noexcept
{
    // try to delete the target file in case it already exists.
    std::remove(target_filename.c_str());
    return std::rename(src_filename.c_str(), target_filename.c_str()) == 0;
}

/**
 * Return file size according to open FILE* object
 */
std::size_t filesize(FILE *f)
{
    if (f == nullptr)
    {
        throw("Failed getting file size. fd is null");
    }
#if defined(_WIN32) && !defined(__CYGWIN__)
    int fd = ::_fileno(f);
#if defined(_WIN64) // 64 bits
    __int64 ret = ::_filelengthi64(fd);
    if (ret >= 0)
    {
        return static_cast<size_t>(ret);
    }

#else // windows 32 bits
    long ret = ::_filelength(fd);
    if (ret >= 0)
    {
        return static_cast<size_t>(ret);
    }
#endif

#else // unix
// OpenBSD and AIX doesn't compile with :: before the fileno(..)
#if defined(__OpenBSD__) || defined(_AIX)
    int fd = fileno(f);
#else
    int fd = ::fileno(f);
#endif
// 64 bits(but not in osx, linux/musl or cygwin, where fstat64 is deprecated)
#if ((defined(__linux__) && defined(__GLIBC__)) || defined(__sun) || defined(_AIX)) && \
    (defined(__LP64__) || defined(_LP64))
    struct stat64 st;
    if (::fstat64(fd, &st) == 0)
    {
        return static_cast<size_t>(st.st_size);
    }
#else // other unix or linux 32 bits or cygwin
    struct stat st;
    if (::fstat(fd, &st) == 0)
    {
        return static_cast<size_t>(st.st_size);
    }
#endif
#endif
    throw("Failed getting file size from fd", errno);
    return 0; // will not be reached.
}

} // namespace Utils
} // namespace dt