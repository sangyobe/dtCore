// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_GENERICCLIENT_H__
#define __DT_RPC_GENERICCLIENT_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcSession.h"

namespace dt
{
namespace Rpc
{

template <typename SendMsgType, typename RecvMsgType>
class GenericClient : public ClientSession
{
public:
    GenericClient(
        void *stub, 
        grpc::CompletionQueue *cq)
        : ClientSession(stub, cq)
    {
    }
    virtual ~GenericClient() = default;

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

#endif // __DT_RPC_GENERICCLIENT_H__