// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_TOPICFEEDER_H__
#define __DT_RPC_TOPICFEEDER_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcClientSession.h"

namespace dt
{
namespace Rpc
{

template <typename MessageType, typename ResponseType>
class TopicFeeder : public ClientSession
{
public:
    TopicFeeder(
        void *stub, 
        grpc::CompletionQueue *cq)
        : ClientSession(stub, cq)
    {
    }
    virtual ~TopicFeeder() = default;

    virtual bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                OnConnect();
                _session_state = SessionState::READY_TO_WRITE;
                return true;
            }
            else if (_session_state == SessionState::WAIT_WRITE_DONE)
            {
                _session_state = SessionState::READY_TO_WRITE;
                return true;
            }
            // else if (_session_state == SessionState::WAIT_FINISH)
            // {
            //     _session_state = SessionState::FINISHED;
            //     return false;
            // }
        }
        return false;
    }

    virtual void OnConnect() {}
    virtual void OnResponse(const ResponseType &response) = 0;

    bool Publish(const MessageType &msg)
    {
        if (_session_state == SessionState::READY_TO_WRITE)
        {
            // std::lock_guard<std::mutex> lock(_proc_mtx);
            _session_state = SessionState::WAIT_WRITE_DONE;
            _topic_writer->Write(msg, &_call_context);
            return true;
        }
        return false;
    }

protected:
    MessageType _message;
    ResponseType _response;
    std::unique_ptr<grpc::ClientAsyncWriter<MessageType>> _topic_writer{nullptr};
    // std::string _topic_name;
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_TOPICFEEDER_H__