// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASOURCE_H__
#define __DTCORE_DTDATASOURCE_H__

/** \defgroup dtDAQ
 *
 */

#include "dtDataSink.h"
#include <memory>
#include <list>

namespace dtCore {

class dtDataSource {
public:
    virtual ~dtDataSource() {}
    void Update() {
        UpdateData();
        UpdateDataSink();
    }
    void AppendDataSink(std::shared_ptr<dtDataSink> sink) {
        _data_sinks.push_back(sink);
    }
protected:
    virtual void UpdateData() = 0;
    virtual void UpdateDataSink() = 0;
protected:
    std::list<std::shared_ptr<dtDataSink>> _data_sinks;
};

}

#endif // __DTCORE_DTDATASOURCE_H__