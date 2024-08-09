// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASINKPB_H__
#define __DT_DAQ_DATASINKPB_H__

/** \defgroup dtDAQ
 *
 */
#include "dtDataSink.h"

namespace dt
{
namespace DAQ
{

template <typename T>
class DataSinkPB : public DataSink
{
public:
    virtual void Publish(T& msg) {}
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASINKPB_H__