// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASOURCE_H__
#define __DT_DAQ_DATASOURCE_H__

/** \defgroup dtDAQ
 *
 */

#include "dtDataSink.h"
#include <list>
#include <memory>

namespace dt
{
namespace DAQ
{

class DataSource
{
public:
    virtual ~DataSource() {}
    void Update(void* context = nullptr) {
        if (UpdateData(context))
            UpdateSink(context);
    }
    void AppendDataSink(std::shared_ptr<DataSink> sink)
    {
        _data_sinks.push_back(sink);
    }

protected:
    virtual bool UpdateData(void* context) = 0;
    virtual void UpdateSink(void* context) = 0;
protected:
    std::list<std::shared_ptr<DataSink>> _data_sinks;
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASOURCE_H__