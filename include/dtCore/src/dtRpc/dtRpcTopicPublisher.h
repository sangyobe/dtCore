// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_TOPICPUBLISHER_H__
#define __DT_RPC_TOPICPUBLISHER_H__

/** \defgroup dtRpc
 *
 */

#include <dtCore/dtThread>
#include "dtRpcServerSession.h"

namespace dt
{
namespace Rpc
{

template <typename RequestType, typename MessageType>
class TopicPublisher : public ServerSession
{
public:
    TopicPublisher(
        Server *server,
        grpc::Service *service, 
        grpc::CompletionQueue* cq_call, 
        grpc::ServerCompletionQueue* cq_ntfy)
        : ServerSession(server, service, cq_call, cq_ntfy)
        , _topic_writer(&_server_ctx)
    {
    }
    virtual ~TopicPublisher() = default;

    bool TryCancelCallAndShutdown() override
    {
        // _run_topic_publisher.store(false);
        // _topic_publisher.join();
        return ServerSession::TryCancelCallAndShutdown();
    }

    bool OnCompletionEvent(bool ok, void *tag) override
    {
        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                OnConnect(); // prepare new session
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    _session_state = SessionState::READY_TO_WRITE;
                }
                
                OnRequest(_request);
                
                // _run_topic_publisher.store(true);
                // _topic_publisher = std::thread([this] (void) {
                //     while (_run_topic_publisher.load())
                //     {
                //         if (UpdateData())
                //             Publish(_message);
                //         dt::Thread::SleepForMillis(10);
                //     }
                // });

                return true;
            }
            else if (_session_state == SessionState::WAIT_WRITE_DONE)
            {
                // LOG(debug) << "TopicPublisher::OnCompletionEvent@WAIT_WRITE_DONE";
                {
                    // std::lock_guard<std::mutex> lock(_proc_mtx);
                    _session_state = SessionState::READY_TO_WRITE;
                }
                return true;
            }
            else if (_session_state == SessionState::WAIT_FINISH)
            {
                // LOG(debug) << "TopicPublisher::OnCompletionEvent@WAIT_FINISH";
                return false;
            }
            else
            {
                // LOG(err) << "TopicPublisher::OnCompletionEvent. Invalid session status (" << static_cast<int>(_session_state) << ")";
                return false;
            }
        }
        return false;
    }

    virtual void OnConnect() = 0;
    virtual void OnRequest(const RequestType &request) = 0;

    bool Publish(const MessageType &msg)
    {
        if (_session_state == SessionState::READY_TO_WRITE)
        {
            // std::lock_guard<std::mutex> lock(_proc_mtx);
            _session_state = SessionState::WAIT_WRITE_DONE;
            _topic_writer.Write(msg, &_call_context);
            return true;
        }
        return false;
    }

protected:
    RequestType _request;
    MessageType _message;
    ::grpc::ServerAsyncWriter<MessageType> _topic_writer;
    // std::string _topic_name;
    CallContext _call_context{this};
    // std::thread _topic_publisher;
    // std::atomic<bool> _run_topic_publisher{false};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_TOPICPUBLISHER_H__