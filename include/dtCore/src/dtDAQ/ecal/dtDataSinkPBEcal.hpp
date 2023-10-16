// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDATASINKPBECAL_H__
#define __DTCORE_DTDATASINKPBECAL_H__

/** \defgroup dtDAQ
 *
 */

#include "../dtDataSinkPB.hpp"
#include <string>
#include <chrono>
#include <ecal/ecal.h>
#include <ecal/msg/protobuf/publisher.h>

//#define PRINT_PUB_SUB_INFO
#ifdef PRINT_PUB_SUB_INFO
#include <iostream>
#endif

namespace dtCore {

template<typename T>
class dtDataSinkPBEcal : public dtDataSinkPB<T> {
public:
    dtDataSinkPBEcal(const std::string& topic_name) : _pub(topic_name) {}
    void Publish(T& msg) {
#ifdef PRINT_PUB_SUB_INFO
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "Publish topic : " << _pub.GetTopicName() << std::endl;
        std::cout << "message type  : " << _pub.GetTypeName() << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "topic id      : " << msg.header().seq() << std::endl;
#endif
        std::chrono::system_clock::time_point start =
            std::chrono::system_clock::now();

        _pub.Send(msg);

        std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
        std::chrono::duration<double> sec = end - start;
#ifdef PRINT_PUB_SUB_INFO
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "elapsed(sec)  : " << sec.count() << std::endl;
        std::cout << "------------------------------------------" << std::endl;
#endif
    }

protected:
    eCAL::protobuf::CPublisher<T> _pub;
};

}

#endif // __DTCORE_DTDATASINKPBECAL_H__