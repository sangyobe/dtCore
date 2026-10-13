// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_GENERICSERVER_H__
#define __DT_RPC_GENERICSERVER_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcServerSession.h"

namespace dt
{
namespace Rpc
{

template <typename SendMsgType, typename RecvMsgType>
class GenericServer : public ServerSession
{
public:
    GenericServer(
        Server *server,
        grpc::Service *service, 
        grpc::CompletionQueue* cq_call, 
        grpc::ServerCompletionQueue* cq_ntfy)
        : ServerSession(server, service, cq_call, cq_ntfy)
    {
    }
    virtual ~GenericServer() = default;

    virtual void OnMessage(const RecvMsgType &)
    {
    }
    virtual void SendMessage(const SendMsgType &)
    {
    }
    virtual bool OnCompletionEvent(bool ok, void *tag) override
    {
        return true;
    }

protected:
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_GENERICSERVER_H__