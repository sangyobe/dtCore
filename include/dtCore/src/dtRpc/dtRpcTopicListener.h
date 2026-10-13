// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_TOPICLISTENER_H__
#define __DT_RPC_TOPICLISTENER_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcServerSession.h"

namespace dt
{
namespace Rpc
{

template <typename MessageType, typename ResponseType>
class TopicListener : public ServerSession
{
public:
    TopicListener(
        Server *server,
        grpc::Service *service, 
        grpc::CompletionQueue* cq_call, 
        grpc::ServerCompletionQueue* cq_ntfy)
        : ServerSession(server, service, cq_call, cq_ntfy)
        , _topic_reader(&_server_ctx)
    {
    }
    virtual ~TopicListener() = default;

    bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                OnConnect(); // prepare new session
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    _session_state = SessionState::WAIT_READ_DONE;
                    _topic_reader.Read(&_message, &_call_context);
                }
                return true;
            }
            else if (_session_state == SessionState::WAIT_READ_DONE)
            {
                // LOG(debug) << "TopicListener::OnCompletionEvent@WAIT_READ_DONE";
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    OnMessage(_message);
                    _session_state = SessionState::WAIT_READ_DONE;
                    _topic_reader.Read(&_message, &_call_context);
                }
                return true;
            }
            else if (_session_state == SessionState::WAIT_FINISH)
            {
                // LOG(debug) << "TopicListener::OnCompletionEvent@WAIT_FINISH";
                return false;
            }
            else
            {
                // LOG(err) << "TopicListener::OnCompletionEvent. Invalid session status (" << static_cast<int>(_session_state) << ")";
                return false;
            }
        }
        return false;
    }

    virtual void OnConnect() = 0;
    virtual void OnMessage(const MessageType &message) = 0;

protected:
    MessageType _message;
    ResponseType _response;
    ::grpc::ServerAsyncReader<ResponseType, MessageType> _topic_reader;
    // std::string _topic_name;
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_TOPICLISTENER_H__