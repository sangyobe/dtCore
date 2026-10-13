// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_SERVICESERVER_H__
#define __DT_RPC_SERVICESERVER_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcServerSession.h"

namespace dt
{
namespace Rpc
{

template <typename RequestType, typename ResponseType>
class ServiceServer : public ServerSession
{
public:
    ServiceServer(
        Server *server,
        grpc::Service *service, 
        grpc::CompletionQueue* cq_call, 
        grpc::ServerCompletionQueue* cq_ntfy)
        : ServerSession(server, service, cq_call, cq_ntfy)
        , _responder(&_server_ctx)
    {
    }
    virtual ~ServiceServer() = default;

    bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                OnConnect(); // prepare new session
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    OnRequest(_request, _response);
                    _session_state = SessionState::WAIT_FINISH;
                    _responder.Finish(_response, grpc::Status::OK, &_call_context);
                }
                return true;
            }
            else if (_session_state == SessionState::WAIT_FINISH)
            {
                // LOG(debug) << "MyServiceServer::OnCompletionEvent@WAIT_FINISH";
                //_session_state = SessionState::FINISHED;
                // _server->RemoveSession(_id);
                return false;
            }
            else
            {
                // LOG(err) << "MyServiceServer::OnCompletionEvent. Invalid session status (" << static_cast<int>(_session_state) << ")";
                // ABSL_ASSERT(false && "Invalid Session Status.");
                return false;
            }
        }
        return false;
    }

    virtual void OnConnect() = 0;
    virtual void OnRequest(const RequestType &request, ResponseType &response) = 0;

protected:
    RequestType _request;
    ResponseType _response;
    ::grpc::ServerAsyncResponseWriter<ResponseType> _responder;
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_SERVICESERVER_H__