// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_DAQMANAGERGRPC_H__
#define __DT_DAQ_DAQMANAGERGRPC_H__

/** ]addtogroup dtDAQ
 *
 */

#include "../dtDAQManager.h"
#include <grpc/grpc.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

namespace dt
{
namespace DAQ
{

class DAQManagerGrpc : public DAQManager
{
public:
    DAQManagerGrpc() {}
    virtual ~DAQManagerGrpc() {}

    void Initialize() {
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    }
    void Terminate() {
    }
private:
};

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_DAQMANAGERECAL_H__