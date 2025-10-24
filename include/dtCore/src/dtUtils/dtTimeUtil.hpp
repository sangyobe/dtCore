// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

/*!
 \file      dtTimeUtil.hpp
 \brief     Time manipulation helper functions.
 \author    Sangyup Yi, sean.yi@hyundai.com
 \date      2025. 01. 22
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef __DT_UTILS_TIMEUTIL_H__
#define __DT_UTILS_TIMEUTIL_H__

/** \addtogroup dtUtils
 *
 */

#include <ctime>
#include <stdexcept>
#include <string>

namespace dt
{
namespace Utils
{

/**
 * get current epoch time.
 */
inline time_t epoch()
{
    time_t ep;
    std::time(&ep);
    return ep;
}

/**
 * convert from datetime to epoch.
 */
inline time_t epoch(std::tm *ts)
{
    time_t ep;
    ep = std::mktime(ts);
    return ep;
}

/**
 * get current local time in datetime format.
 */
inline std::tm localtime()
{
    time_t ep;
    std::tm ts;
    std::time(&ep);
    ts = *std::localtime(&ep);
    return ts;
}

/**
 * convert from custom epoch to datetime.
 */
inline std::tm localtime(const time_t *t)
{
    std::tm ts;
    ts = *std::localtime(t);
    return ts;
}

/**
 * get current local time string.
 * returns length of written bytes to buf.
 * returns 0 if function call fails.
 */
inline size_t localtimestr(char *buf, size_t buf_len)
{
    time_t ep;
    std::tm ts;
    size_t len;
    std::time(&ep);
    ts = *std::localtime(&ep);
    len = strftime(buf, buf_len, "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    return len;
}

/**
 * get local time string from epoch time given.
 * returns length of written bytes to buf.
 * returns 0 if function call fails.
 */
inline size_t localtimestr(const time_t *t, char *buf, size_t buf_len)
{
    std::tm ts;
    size_t len;
    ts = *std::localtime(t);
    len = strftime(buf, buf_len, "%a %Y-%m-%d %H:%M:%S %Z", &ts);
    return len;
}

/**
 * get local time string from datetime given.
 * returns length of written bytes to buf.
 * returns 0 if function call fails.
 */
inline size_t localtimestr(const std::tm *ts, char *buf, size_t buf_len)
{
    size_t len;
    len = strftime(buf, buf_len, "%a %Y-%m-%d %H:%M:%S %Z", ts);
    return len;
}

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_TIMEUTIL_H__