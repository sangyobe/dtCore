// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASOURCEPB_H__
#define __DTCORE_DTDATASOURCEPB_H__

/** \defgroup dtDAQ
 *
 */

#include <dtCore/define.h>
#include "dtDataSource.h"
#include "dtDataSinkPB.hpp"

namespace dtCore {

template<typename T>
class dtDataSourcePB : public dtDataSource {
protected:
    bool UpdateData(void* context) { UNUSED(context); return false; }
    void UpdateSink(void* context) {
        UNUSED(context); 
        for (const std::shared_ptr<dtDataSink>& sink : _data_sinks) {
            std::shared_ptr<dtDataSinkPB<T>> s = std::dynamic_pointer_cast<dtDataSinkPB<T>>(sink);
            if (s) {
                s->Publish(_msg);
            }
        }
    }
protected:
    T _msg;
};

template<typename T>
class dtDataSourcePBTimestamped : public dtDataSourcePB<T> {
protected:
    void UpdateTimestamp() {
        this->_msg.mutable_header()->set_seq(++_cnt);
#ifdef _WIN32
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);
        UINT64 ticks = (((UINT64)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        // A Windows tick is 100 nanoseconds. Windows epoch 1601-01-01T00:00:00Z
        // is 11644473600 seconds before Unix epoch 1970-01-01T00:00:00Z.
        this->_msg.mutable_header()->mutable_time_stamp()->set_seconds((INT64)((ticks / 10000000) - 11644473600LL));
        this->_msg.mutable_header()->mutable_time_stamp()->set_nanos((INT32)((ticks % 10000000) * 100));
#else
        struct timespec tp;
        clock_gettime(CLOCK_REALTIME, &tp);
        this->_msg.mutable_header()->mutable_time_stamp()->set_seconds(tp.tv_sec);
        this->_msg.mutable_header()->mutable_time_stamp()->set_nanos(tp.tv_nsec);
#endif
    }
protected:
    uint32_t _cnt {0};
};

}

#endif // __DTCORE_DTDATASOURCEPB_H__