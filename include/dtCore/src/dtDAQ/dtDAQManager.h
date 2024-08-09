// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DAQMANAGER_H__
#define __DT_DAQ_DAQMANAGER_H__

/** \defgroup dtDAQ
 *
 */

#include "dtDataSource.h"
namespace dt
{
namespace DAQ
{

class DAQManager
{
public:
    virtual ~DAQManager() {}

    virtual void Initialize() = 0;
    virtual void Terminate() = 0;
    virtual void AppendDataSource(std::shared_ptr<DataSource> src)
    {
        _data_srcs.push_back(src);
    }
    virtual void Update(void* context = nullptr)
    {
        for (const std::shared_ptr<DataSource> &src : _data_srcs)
        {
            src->Update(context);
        }
    }

protected:
    std::list<std::shared_ptr<DataSource>> _data_srcs;
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DAQMANAGER_H__