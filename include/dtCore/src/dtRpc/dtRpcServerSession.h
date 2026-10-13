// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_SERVERSESSION_H__
#define __DT_RPC_SERVERSESSION_H__

/** \defgroup dtRpc
 *
 */

#include <atomic>
#include <grpcpp/grpcpp.h>
#include "dtRpcSession.h"

namespace dt
{
namespace Rpc
{

class Server;

class ServerSession : public Session
{
public:
    ServerSession(
        Server *server,
        grpc::Service *service,
        grpc::CompletionQueue *cq_call,
        grpc::ServerCompletionQueue *cq_ntfy)
        : Session()
        , _server(server)
        , _service(service)
        , _cq_call(cq_call)
        , _cq_ntfy(cq_ntfy)
    {
    }
    virtual ~ServerSession() = default;

    bool TryCancelCallAndShutdown() override
    {
        // std::lock_guard<std::mutex> lock(_proc_mtx);
        if (_session_state != SessionState::WAIT_CONNECT &&
            _session_state != SessionState::WAIT_FINISH &&
            _session_state != SessionState::FINISHED)
        {
            _server_ctx.TryCancel();

            // std::lock_guard<std::mutex> lock(_proc_mtx);
            //_session_state = SessionState::WAIT_FINISH;
            _session_state = SessionState::FINISHED;
        }

        // _session_state = SessionState::FINISHED;
        // _server->RemoveSession(_id);

        return true;
    }

    void StreamData() override
    {
    }

protected:
    Server *_server{nullptr};
    grpc::Service *_service{nullptr};
    grpc::CompletionQueue *_cq_call{nullptr};
    grpc::ServerCompletionQueue *_cq_ntfy{nullptr};
    grpc::ServerContext _server_ctx; // ServerContext instances should not be reused across rpcs!
    // std::mutex _proc_mtx;

    enum class SessionState
    {
        WAIT_CONNECT,
        READY_TO_READ,
        WAIT_READ_DONE,
        READY_TO_WRITE,
        WAIT_WRITE_DONE,
        WAIT_FINISH,
        FINISHED,
        PEER_DISCONNECTED
    };
    std::atomic<SessionState> _session_state{SessionState::WAIT_CONNECT};

private: // deleted default copy&assignment constructors
    ServerSession(const ServerSession &) = delete;
    ServerSession(ServerSession &&) = delete;
    ServerSession &operator=(const ServerSession &) = delete;
    ServerSession &operator=(ServerSession &&) = delete;
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_SERVERSESSION_H__