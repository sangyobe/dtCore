//* Related Headers ----------------------------------------------------------*/
#include "dtCore/src/dtUtils/dtDir.h"

//* C/C++ System Headers -----------------------------------------------------*/
#include <memory>
#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>

//* Other Lib Headers --------------------------------------------------------*/
//* Project Headers ----------------------------------------------------------*/
//* System-Specific Headers --------------------------------------------------*/

//* Public(Exported) Variables -----------------------------------------------*/
//* Private Macro ------------------------------------------------------------*/
//* Private Types ------------------------------------------------------------*/
//* Private Variables --------------------------------------------------------*/
//* Private Functions --------------------------------------------------------*/
//* Public(Exported) Functions Definition ------------------------------------*/
// returns the path to the directory containing the current executable
std::string GetExecutableDir()
{
#if defined(_WIN32) || defined(__CYGWIN__)
    constexpr char kPathSep = '\\';
    std::string realpath = [&]() -> std::string {
        std::unique_ptr<char[]> realpath(nullptr);
        DWORD buf_size = 128;
        bool success = false;
        while (!success)
        {
            realpath.reset(new (std::nothrow) char[buf_size]);
            if (!realpath)
            {
                fprintf(stderr, "cannot allocate memory to store executable path\n");
                return "";
            }

            DWORD written = GetModuleFileNameA(nullptr, realpath.get(), buf_size);
            if (written < buf_size)
            {
                success = true;
            }
            else if (written == buf_size)
            {
                // realpath is too small, grow and retry
                buf_size *= 2;
            }
            else
            {
                fprintf(stderr, "failed to retrieve executable path: %d\n", GetLastError());
                return "";
            }
        }
        return realpath.get();
    }();
#else
    constexpr char kPathSep = '/';
#if defined(__APPLE__)
    std::unique_ptr<char[]> buf(nullptr);
    {
        std::uint32_t buf_size = 0;
        _NSGetExecutablePath(nullptr, &buf_size);
        buf.reset(new char[buf_size]);
        if (!buf)
        {
            fprintf(stderr, "cannot allocate memory to store executable path\n");
            return "";
        }
        if (_NSGetExecutablePath(buf.get(), &buf_size))
        {
            fprintf(stderr, "unexpected error from _NSGetExecutablePath\n");
        }
    }
    const char *path = buf.get();
#else
    const char *path = "/proc/self/exe";
#endif
    std::string realpath = [&]() -> std::string {
        std::unique_ptr<char[]> realpath(nullptr);
        std::uint32_t buf_size = 128;
        bool success = false;
        while (!success)
        {
            realpath.reset(new (std::nothrow) char[buf_size]);
            if (!realpath)
            {
                fprintf(stderr, "cannot allocate memory to store executable path\n");
                return "";
            }

            std::size_t written = readlink(path, realpath.get(), buf_size);
            if (written < buf_size)
            {
                realpath.get()[written] = '\0';
                success = true;
            }
            else if (written == -1)
            {
                if (errno == EINVAL)
                {
                    // path is already not a symlink, just use it
                    return path;
                }

                fprintf(stderr, "error while resolving executable path: %s\n", strerror(errno));
                return "";
            }
            else
            {
                // realpath is too small, grow and retry
                buf_size *= 2;
            }
        }
        return realpath.get();
    }();
#endif

    if (realpath.empty())
    {
        return "";
    }

    for (std::size_t i = realpath.size() - 1; i > 0; --i)
    {
        if (realpath.c_str()[i] == kPathSep)
        {
            return realpath.substr(0, i);
        }
    }

    // don't scan through the entire file system's root
    return "";
}

// returns the directory where tasks are stored
std::string GetTasksDir()
{
    const char *tasks_dir = std::getenv("USERCTRL_TASK_DIR");
    if (tasks_dir)
    {
        return tasks_dir;
    }
    return GetExecutableDir().substr(0, GetExecutableDir().find_last_of("/\\", GetExecutableDir().find_last_of("/\\") - 1));
}

// returns path to a model XML file given path relative to models dir
std::string GetModelPath(std::string path)
{
    return GetTasksDir() + "/model/" + std::string(path);
}

// return file extension for the given file
std::string GetFileExtension(const std::string &filePath)
{
    size_t pos = filePath.rfind('.');
    if (pos != std::string::npos)
    {
        return filePath.substr(pos);
    }
    else
    {
        return ""; // No extension found
    }
}

// returns the current user's home directory
std::string GetUserHomeDir()
{
#if defined(_MSC_VER)
#if defined(__cplusplus_winrt)
    return std::string{}; // not supported under uwp
#else
    size_t len = 0;
    char buf[128];
    bool ok = ::getenv_s(&len, buf, sizeof(buf), "HOME") == 0;
    return ok ? buf : std::string{};
#endif
#else // revert to getenv
    char *buf = ::getenv("HOME");
    return buf ? buf : std::string{};
#endif
}