// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASINKPB_H__
#define __DTCORE_DTDATASINKPB_H__

/** \defgroup dtDAQ
 *
 */
#include "dtDataSink.h"

namespace dtCore {

template<typename T>
class dtDataSinkPB : public dtDataSink {
public:
    virtual void Publish(T& msg) {}
};

}

#endif // __DTCORE_DTDATASINKPB_H__