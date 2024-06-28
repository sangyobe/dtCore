// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_TYPE_H__
#define __DTCORE_TYPE_H__

namespace dt
{

#ifdef _WIN32
typedef struct _timeStamp
{
    UINT64 tv_sec;  /* Seconds. */
    UINT64 tv_nsec; /* Nanoseconds. */
} TimeStamp;
#else
#include <time.h>
typedef struct timespec TimeStamp;
#endif

} // namespace dt

#endif //__DTCORE_TYPE_H__
