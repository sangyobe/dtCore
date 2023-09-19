// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASINKPBGRPC_H__
#define __DTCORE_DTDATASINKPBGRPC_H__

/** \defgroup dtDAQ
 *
 */

#include "../dtDataSinkPB.hpp"

namespace dtCore {

template<typename T>
class dtDataSinkPBGrpc : public dtDataSinkPB<T> {
public:
    dtDataSinkPBGrpc(const std::string& topic_name) : _pub(topic_name) {}
    void Update(T& msg) {
    }
};

}

#endif // __DTCORE_DTDATASINKPBGRPC_H__