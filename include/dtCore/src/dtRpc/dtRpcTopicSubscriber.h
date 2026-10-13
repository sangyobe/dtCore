// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_TOPICSUBSCRIBER_H__
#define __DT_RPC_TOPICSUBSCRIBER_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcClientSession.h"

namespace dt
{
namespace Rpc
{

template <typename RequestType, typename MessageType>
class TopicSubscriber : public ClientSession
{
public:
    TopicSubscriber(
        void *stub, 
        grpc::CompletionQueue *cq)
        : ClientSession(stub, cq)
    {
    }
    virtual ~TopicSubscriber() = default;

    virtual bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                OnConnect();
                _topic_reader->Read(&_message, &_call_context);
                _session_state = SessionState::WAIT_READ_DONE;
                return true;
            }
            else if (_session_state == SessionState::WAIT_READ_DONE)
            {
                OnMessage(_message);

                _session_state = SessionState::WAIT_READ_DONE;
                _topic_reader->Read(&_message, &_call_context);
                return true;
            
            }
            else if (_session_state == SessionState::WAIT_FINISH)
            {
                _session_state = SessionState::FINISHED;
                return false;
            }
        }
        return false;
    }

    virtual void OnConnect() {}
    virtual void OnMessage(const MessageType &message) = 0;

protected:
    RequestType _request;
    MessageType _message;
    std::unique_ptr<grpc::ClientAsyncReader<MessageType>> _topic_reader{nullptr};
    // std::string _topic_name;
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_TOPICSUBSCRIBER_H__