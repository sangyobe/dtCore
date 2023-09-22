// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDAQMANAGER_H__
#define __DTCORE_DTDAQMANAGER_H__

/** \defgroup dtDAQ
 *
 */

#include "dtDataSource.h"

namespace dtCore {

class dtDAQManager {
public:
    virtual ~dtDAQManager() {}

    virtual void Initialize() = 0;
    virtual void Terminate() = 0;
    virtual void AppendDataSource(std::shared_ptr<dtDataSource> src)
    {
        _data_srcs.push_back(src);
    }
    virtual void Update()
    {
        for(const std::shared_ptr<dtDataSource>& src : _data_srcs) {
            src->Update();
        }
    }

protected:
    std::list<std::shared_ptr<dtDataSource>> _data_srcs;
};

}

#endif // __DTCORE_DTDAQMANAGER_H__