// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASINKPBGRPC_H__
#define __DT_DAQ_DATASINKPBGRPC_H__

/** ]addtogroup dtDAQ
 *
 */

#include "../dtDataSinkPB.hpp"

namespace dt
{
namespace DAQ
{

template <typename T>
class DataSinkPBGrpc : public DataSinkPB<T>
{
public:
    DataSinkPBGrpc(const std::string &topic_name) : _pub(topic_name) {}
    void Publish(T& msg) {
        std::cout << "DataSinkPBGrpc::Publish(T&)" << std::endl;
    }

protected:
    std::string _pub;
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASINKPBGRPC_H__