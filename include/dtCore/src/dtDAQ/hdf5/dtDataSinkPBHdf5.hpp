// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASINKPBHDF5_H__
#define __DTCORE_DTDATASINKPBHDF5_H__

/** \defgroup dtDAQ
 *
 */

#include <iostream>
#include <string>

#include "../dtDataSinkPB.hpp"

namespace dtCore {

template<typename T>
class dtDataSinkPBHdf5 : public dtDataSinkPB<T>
{
public:
    dtDataSinkPBHdf5(const std::string& file_basename = "", bool annot_datetime = true) {}
    ~dtDataSinkPBHdf5() {}

    void Publish(T& msg) { std::cerr << "dtDataSinkPBHdf5: Not implemented." << std::endl; }
};

}

#endif // __DTCORE_DTDATASINKPBHDF5_H__