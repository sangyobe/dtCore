// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DATASINKPBHDF5_H__
#define __DT_DAQ_DATASINKPBHDF5_H__

/** \defgroup dtDAQ
 *
 */

#include <iostream>
#include <string>

#include "../dtDataSinkPB.hpp"

namespace dt
{
namespace DAQ
{

template <typename T>
class DataSinkPBHdf5 : public DataSinkPB<T>
{
public:
    DataSinkPBHdf5(const std::string &file_basename = "", bool annot_datetime = true) {}
    ~DataSinkPBHdf5() {}

    void Publish(T &msg) { std::cerr << "DataSinkPBHdf5: Not implemented." << std::endl; }
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DATASINKPBHDF5_H__