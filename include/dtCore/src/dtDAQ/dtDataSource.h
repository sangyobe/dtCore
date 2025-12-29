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

#include <thread>
#include <mutex>
#include <list>
#include <memory>
#include "dtDataSink.h"

namespace dt
{
namespace DAQ
{

class DataSource
{
public:
    virtual ~DataSource() {}
    void Update(void* context = nullptr) 
    {
        if (UpdateData(context))
        {
            std::lock_guard<std::mutex> lock(_sink_mtx);
            UpdateSink(context);
        }
    }
    void AppendDataSink(std::shared_ptr<DataSink> sink)
    {
        std::lock_guard<std::mutex> lock(_sink_mtx);
        _data_sinks.push_back(sink);
    }
    void RemoveDataSink(std::shared_ptr<DataSink> sink)
    {
        std::lock_guard<std::mutex> lock(_sink_mtx);
        _data_sinks.remove(sink);
    }

protected:
    virtual bool UpdateData(void* context) = 0;
    virtual void UpdateSink(void* context) = 0;
protected:
    std::list<std::shared_ptr<DataSink>> _data_sinks;
    std::mutex _sink_mtx;
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASOURCE_H__