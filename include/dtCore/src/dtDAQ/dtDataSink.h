// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASINK_H__
#define __DTCORE_DTDATASINK_H__

/** \defgroup dtDAQ
 *
 */

namespace dtCore {

class dtDataSink {
public:
    dtDataSink() {}
    virtual ~dtDataSink() {}
    virtual void Publish() {}
};

}

#endif // __DTCORE_DTDATASINK_H__