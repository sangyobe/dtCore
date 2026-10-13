// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_CLIENTSESSION_H__
#define __DT_RPC_CLIENTSESSION_H__

/** \defgroup dtRpc
 *
 */

#include <grpcpp/grpcpp.h>
#include "dtRpcSession.h"

namespace dt
{
namespace Rpc
{

class ClientSession : public Session
{
public:
    ClientSession(
        void *stub, 
        grpc::CompletionQueue *cq)
        : Session()
        , _stub(stub)
        , _cq(cq)
        , _session_state(SessionState::WAIT_CONNECT)
    {
    }
    virtual ~ClientSession() = default;

    bool TryCancelCallAndShutdown() override
    {
        // std::lock_guard<std::mutex> lock(_proc_mtx);
        if (_session_state != SessionState::WAIT_CONNECT &&
            _session_state != SessionState::WAIT_FINISH &&
            _session_state != SessionState::FINISHED)
        {
            _client_ctx.TryCancel();

            // std::lock_guard<std::mutex> lock(_proc_mtx);
            //_session_state = SessionState::WAIT_FINISH;
            _session_state = SessionState::FINISHED;
        }

        return true;
    }

    void StreamData() override
    {
    }

    enum class SessionState
    {
        WAIT_CONNECT,
        WAIT_RESPONSE,
        READY_TO_READ,
        WAIT_READ_DONE,
        READY_TO_WRITE,
        WAIT_WRITE_DONE,
        WAIT_FINISH,
        FINISHED,
        PEER_DISCONNECTED
    };

protected:
    void *_stub{nullptr};
    grpc::CompletionQueue *_cq{nullptr};
    grpc::ClientContext _client_ctx;
    grpc::Status _call_status;
    // std::mutex _proc_mtx;
    
    SessionState _session_state;

private: // deleted default copy&assignment constructors
    ClientSession(const ClientSession &) = delete;
    ClientSession(ClientSession &&) = delete;
    ClientSession &operator=(const ClientSession &) = delete;
    ClientSession &operator=(ClientSession &&) = delete;
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_CLIENTSESSION_H__