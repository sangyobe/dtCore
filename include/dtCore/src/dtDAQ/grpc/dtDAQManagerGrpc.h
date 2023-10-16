// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTDAQMANAGERGRPC_H__
#define __DTCORE_DTDAQMANAGERGRPC_H__

/** \defgroup dtDAQ
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

namespace dtCore {

class dtDAQManagerGrpc : public dtDAQManager {
public:
    dtDAQManagerGrpc() {}
    virtual ~dtDAQManagerGrpc() {}

    void Initialize() {
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    }
    void Terminate() {
    }
private:
};

}

#endif // __DTCORE_DTDAQMANAGERECAL_H__