// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_SERVICECLIENT_H__
#define __DT_RPC_SERVICECLIENT_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcClientSession.h"

namespace dt
{
namespace Rpc
{

template <typename RequestType, typename ResponseType>
class ServiceClient : public ClientSession
{
public:
    ServiceClient(
        void *stub, 
        grpc::CompletionQueue *cq)
        : ClientSession(stub, cq)
    {
    }
    virtual ~ServiceClient() = default;

    virtual bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (!_call_status.ok())
        {
            // std::cout << "ServiceClient: rpc failed with error code(" << _call_status.error_code() << "): " 
            //           << _call_status.error_message() << std::endl;
            _session_state = SessionState::FINISHED;
            return false;
        }

        if (ok)
        {
            if (_session_state == SessionState::WAIT_FINISH)
            {
                OnResponse(_response);
                _session_state = SessionState::FINISHED;
                return false;
            }
        }

        _session_state = SessionState::FINISHED;
        return false;
    }

    virtual void OnResponse(const ResponseType &response) = 0;

    SessionState GetSessionState() { return _session_state; }
    ResponseType GetResponse() { return _response; }

protected:
    RequestType _request;
    ResponseType _response;
    std::unique_ptr<grpc::ClientAsyncResponseReader<ResponseType>> _response_reader{nullptr};
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_SERVICECLIENT_H__