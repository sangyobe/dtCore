// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASINK_H__
#define __DT_DAQ_DATASINK_H__

/** \addtogroup dtDAQ
 *
 */

namespace dt
{
namespace DAQ
{

class DataSink
{
public:
    DataSink() {}
    virtual ~DataSink() {}
    virtual void Publish() {}
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASINK_H__