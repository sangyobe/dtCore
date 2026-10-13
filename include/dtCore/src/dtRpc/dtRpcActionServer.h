// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_ACTIONSERVER_H__
#define __DT_RPC_ACTIONSERVER_H__

/** \defgroup dtRpc
 *
 */

#include <dtCore/dtLog>
#include "dtRpcServerSession.h"

namespace dt
{
namespace Rpc
{

template <typename RequestType, typename ResponseType>
class ActionServer : public ServerSession
{
public:
    ActionServer(
        Server *server,
        grpc::Service *service, 
        grpc::CompletionQueue* cq_call, 
        grpc::ServerCompletionQueue* cq_ntfy)
        : ServerSession(server, service, cq_call, cq_ntfy)
        , _responder(&_server_ctx)
    {
    }
    virtual ~ActionServer() = default;

    bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                LOG(debug) << "ActionServer::OnCompletionEvent@WAIT_CONNECT";
                OnConnect(); // prepare new session
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    _session_state = SessionState::READY_TO_WRITE;
                }
                OnRequest(_request);
                return true;
            }
            else if (_session_state == SessionState::WAIT_WRITE_DONE)
            {
                LOG(debug) << "ActionServer::OnCompletionEvent@WAIT_WRITE_DONE";
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    _session_state = SessionState::READY_TO_WRITE;
                }
                return true;
            }
            else if (_session_state == SessionState::WAIT_FINISH)
            {
                LOG(debug) << "ActionServer::OnCompletionEvent@WAIT_FINISH";
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    _session_state = SessionState::FINISHED;
                    _responder.Finish(grpc::Status::OK, &_call_context);
                }
                return true;
                // return false;
            }
            else if (_session_state == SessionState::FINISHED)
            {
                LOG(debug) << "ActionServer::OnCompletionEvent@FINISHED";
                return false;
            }
            else
            {
                LOG(err) << "ActionServer::OnCompletionEvent. Invalid session status (" << static_cast<int>(_session_state.load()) << ")";
                return false;
            }
        }
        return false;
    }

    virtual void OnConnect() = 0;
    virtual void OnRequest(const RequestType &request) = 0;

    bool Feedback(const ResponseType &feedback)
    {
        if (_session_state == SessionState::READY_TO_WRITE)
        {
            // std::lock_guard<std::mutex> lock(_proc_mtx);
            _session_state = SessionState::WAIT_WRITE_DONE;
            _responder.Write(feedback, &_call_context);
            return true;
        }
        return false;
    }

    bool Respond(const ResponseType &response)
    {
        if (_session_state == SessionState::READY_TO_WRITE)
        {
            // std::lock_guard<std::mutex> lock(_proc_mtx);
            // _server_ctx.TryCancel();
            _session_state = SessionState::WAIT_FINISH;
            _responder.Write(response, &_call_context);
            // _responder.WriteAndFinish(response, grpc::WriteOptions(), grpc::Status::OK, &_call_context);
            return true;
        }
        return false;
    }

protected:
    RequestType _request;
    ResponseType _response;
    ::grpc::ServerAsyncWriter<ResponseType> _responder;
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_ACTIONSERVER_H__