// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

/*!
 \file      dtStringHelper.h
 \brief     String manipulation helper functions.
 \author    Sangyup Yi, sean.yi@hyundai.com
 \date      2024. 04. 23
 \version   1.0.0
 \copyright RoboticsLab ART All rights reserved.
*/

#ifndef __DT_UTILS_STRINGHELPER_H__
#define __DT_UTILS_STRINGHELPER_H__

/** \defgroup dtUtils
 *
 */

#include <string>
#include <stdexcept>

namespace dt
{
namespace Utils
{

template<typename ... Args>
std::string string_format(const std::string& format, Args ... args)
{
	size_t size = snprintf(nullptr, 0, format.c_str(), args ...) + 1; // Extra space for '\0'
	if (size <= 0) { throw std::runtime_error("Error during formatting."); }
	// std::unique_ptr<char[]> buf(new char[size]);
	// snprintf(buf.get(), size, format.c_str(), args ...);
	// return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
    char* buf = (char*)alloca(size);
    snprintf(buf, size, format.c_str(), args ...);
	return std::string(buf, buf + size - 1); // We don't want the '\0' inside
}

} // namespace Utils
} // namespace dt

#endif // __DT_UTILS_STRINGHELPER_H__