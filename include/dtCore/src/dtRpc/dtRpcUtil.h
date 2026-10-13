// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_UTIL_H__
#define __DT_RPC_UTIL_H__

/** \defgroup dtRpc
 *
 */

#include <ctime>
#include <stdexcept>
#include <string>
#include <dtCore/dtLog>
#include <google/protobuf/timestamp.pb.h>

namespace dt
{
namespace Rpc
{

inline void touchTimestamp(google::protobuf::Timestamp *ts)
{
#ifdef _WIN32
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    UINT64 ticks = (((UINT64)ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
    // A Windows tick is 100 nanoseconds. Windows epoch 1601-01-01T00:00:00Z
    // is 11644473600 seconds before Unix epoch 1970-01-01T00:00:00Z.
    ts->set_seconds((INT64)((ticks / 10000000) - 11644473600LL));
    ts->set_nanos((INT32)((ticks % 10000000) * 100));
#else
    struct timespec tp;
    clock_gettime(CLOCK_REALTIME, &tp);
    ts->set_seconds(tp.tv_sec);
    ts->set_nanos(tp.tv_nsec);
#endif
}

// template <typename HdrType>
// inline void updateMsgHdr(HdrType *hdr)
// {
//     hdr->set_seq(hdr->seq() + 1);
//     touchTimestamp(hdr->mutable_timestamp());
// }

inline std::string getReadablePeerAddress(const std::string &peer)
{
    std::string result = peer;
    size_t pos = 0;
    while ((pos = result.find("%5B", pos)) != std::string::npos)
    {
        result.replace(pos, 3, "[");
    }
    pos = 0;
    while ((pos = result.find("%5D", pos)) != std::string::npos)
    {
        result.replace(pos, 3, "]");
    }

    if (result.compare(0, 5, "ipv4:") == 0 || result.compare(0, 5, "ipv6:") == 0)
    {
        result.erase(0, 5);
    }
    return result;
}

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_UTIL_H__