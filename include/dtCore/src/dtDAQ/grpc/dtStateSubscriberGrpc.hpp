// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_STATESUBSCRIBERGRPC_H__
#define __DT_DAQ_STATESUBSCRIBERGRPC_H__

/** \defgroup dtDAQ
 *
 */

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "dtProto/Service.grpc.pb.h"

#define USE_THREAD_PTHREAD

namespace dt
{
namespace DAQ
{

template <typename StateType>
class StateSubscriberGrpc
{
public:
    StateSubscriberGrpc(const std::string &topic_name, const std::string &server_address);
    ~StateSubscriberGrpc();

    void RegMessageHandler(std::function<void(StateType&)>);
    bool IsRunning();
    void Reconnect();

private:
    std::string _topic_name{""};
    std::string _server_address{""};
    std::function<void(StateType&)> _msg_handler {nullptr};
    std::atomic<bool> _running {true};

    class Session {
    public:
        Session(StateSubscriberGrpc<StateType> *subscriber);
        ~Session();

    private:
        bool InitRequest();
        bool OnCompletionEvent();
        bool TryCancelCallAndShutdown();
        void Stop();

    private:
        std::unique_ptr<dtproto::dtService::Stub> _stub {nullptr};
        grpc::ClientContext _ctx;
        grpc::CompletionQueue _cq;
        grpc::Status _status;
        std::unique_ptr<grpc::ClientAsyncReader<dtproto::std_msgs::State>> _stream_reader;
        dtproto::std_msgs::State _msg;
        enum class RpcCallState {
            WAIT_START,
            READY_TO_READ,
            WAIT_READ_DONE,
            WAIT_FINISH,
            FINISHED,
            PEER_DISCONNECTED
        };
        RpcCallState _call_state {RpcCallState::WAIT_START};
#ifdef USE_THREAD_PTHREAD
        pthread_t _rpc_recv_thread;
#else
        std::thread _rpc_recv_thread;
#endif
        std::mutex _call_mtx;
        StateSubscriberGrpc<StateType> *_subscriber;
    };

    std::unique_ptr<Session> _session{nullptr};
};

template <typename StateType>
StateSubscriberGrpc<StateType>::StateSubscriberGrpc(const std::string &topic_name, const std::string &server_address)
    : _topic_name(topic_name), _server_address(server_address)
{
    _session = std::make_unique<Session>(this);
}

template <typename StateType>
StateSubscriberGrpc<StateType>::~StateSubscriberGrpc()
{
    if(_session) _session.reset();
}

template <typename StateType>
void StateSubscriberGrpc<StateType>::RegMessageHandler(std::function<void(StateType &)> handler)
{
    _msg_handler = handler;
}

template <typename StateType>
bool StateSubscriberGrpc<StateType>::IsRunning()
{
    return _running.load();
}

template <typename StateType>
void StateSubscriberGrpc<StateType>::Reconnect()
{
    if (_session) {
        _session.reset();
    }
    _session = std::make_unique<Session>(this);
}

template <typename StateType>
StateSubscriberGrpc<StateType>::Session::Session(StateSubscriberGrpc<StateType> *subscriber)
    : _subscriber(subscriber)
{
    _stub = dtproto::dtService::NewStub(
        grpc::CreateChannel(_subscriber->_server_address, grpc::InsecureChannelCredentials()));

    InitRequest();
}

template <typename StateType>
StateSubscriberGrpc<StateType>::Session::~Session()
{
    Stop();
}

template <typename StateType>
bool StateSubscriberGrpc<StateType>::Session::InitRequest()
{
    _ctx.set_wait_for_ready(false);

    dtproto::std_msgs::Request req;
    req.set_name(_subscriber->_topic_name);
    req.set_type(dtproto::std_msgs::Request::ON);
    _stream_reader =
        _stub->PrepareAsyncPublishState(
            //&_context,
            &_ctx,
            req,
            &_cq);

    _call_state = RpcCallState::WAIT_START;
    _stream_reader->StartCall((void*)this);
    this->_subscriber->_running = true;

#ifdef USE_THREAD_PTHREAD
    pthread_create(
        &_rpc_recv_thread, NULL,
        [](void *arg) -> void * {
            StateSubscriberGrpc<StateType>::Session *client = (StateSubscriberGrpc<StateType>::Session *)arg;

            // LOG(INFO) << "RPC new-call handler()";
            void *tag;
            bool ok;
            while (client->_cq.Next(&tag, &ok))
            {
                // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
                if (ok && static_cast<StateSubscriberGrpc<StateType>::Session *>(tag)->OnCompletionEvent())
                {
                    continue;
                }
                else
                {
                    static_cast<StateSubscriberGrpc<StateType>::Session *>(tag)->TryCancelCallAndShutdown();
                    break;
                }
            }
            // LOG(INFO) << "RPC handler() exits.";
            client->_subscriber->_running = false;
            return 0;
        },
        (void *)this);
#else
    _rpc_recv_thread = std::thread([this] {
        // LOG(INFO) << "RPC new-call handler()";
        void* tag;
        bool ok;
        while (_cq.Next(&tag, &ok)) {
            // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
            if (ok && static_cast<StateSubscriberGrpc<StateType>::Session *>(tag)->OnCompletionEvent())
            {
                continue;
            }
            else {
                static_cast<StateSubscriberGrpc<StateType>::Session *>(tag)->TryCancelCallAndShutdown();
                break;
            }
        }
        // LOG(INFO) << "RPC handler() exits.";
        this->_subscriber->_running = false;
    });
#endif

    return true;
}

template <typename StateType>
bool StateSubscriberGrpc<StateType>::Session::OnCompletionEvent()
{
    std::lock_guard<std::mutex> lock(_call_mtx);

    if (_call_state == RpcCallState::WAIT_START) {

        if (!_status.ok()) {
            // LOG(INFO) << "SubscribeState rpc call failed.";
            return false;
        }

        // LOG(INFO) << "SubscribeState rpc call started.";

        _stream_reader->Read(&_msg, (void*)this);
        _call_state = RpcCallState::WAIT_READ_DONE;

    }
    else if (_call_state == RpcCallState::WAIT_READ_DONE) {

        if (!_status.ok()) {
            std::cout << "SubscribeState rpc stream broken." << std::endl;
            return false;
        }

        if (_subscriber->_msg_handler) {
            // LOG(INFO) << "Got a new message. Type = " << _msg.state().type_url();
            StateType state;
            _msg.state().UnpackTo(&state);
            _subscriber->_msg_handler(state);
        }

        _stream_reader->Read(&_msg, (void*)this);
        _call_state = RpcCallState::WAIT_READ_DONE;

    }
    else if (_call_state == RpcCallState::WAIT_FINISH) {
        // LOG(INFO) << "Finalize SubscribeState() service call.";
        _call_state = RpcCallState::FINISHED;
        // Once we're complete, deallocate the call object.
        //delete this;
        return false;
    }
    else {
        // LOG(INFO) << "Session::OnCompletionEvent, _call_state = " << static_cast<int>(_call_state);
        return false;
    }

    return true;
}

template <typename StateType>
bool StateSubscriberGrpc<StateType>::Session::TryCancelCallAndShutdown()
{
    std::lock_guard<std::mutex> lock(_call_mtx);

    _ctx.TryCancel();

    if (_call_state != RpcCallState::WAIT_START &&
        _call_state != RpcCallState::WAIT_FINISH &&
        _call_state != RpcCallState::FINISHED) 
    {
        // LOG(INFO) << "Finishing...";
        _call_state = RpcCallState::WAIT_FINISH;
        _stream_reader->Finish(&_status, (void *)this);
    }

    return true;
}

template <typename StateType>
void StateSubscriberGrpc<StateType>::Session::Stop()
{
    TryCancelCallAndShutdown();
    _cq.Shutdown();
    // LOG(INFO) << "CQ shutdown.";
#ifdef USE_THREAD_PTHREAD
        if (_rpc_recv_thread)
        {
            pthread_join(_rpc_recv_thread, nullptr);
            _rpc_recv_thread = (pthread_t)nullptr;
        }
#else
        if (_rpc_recv_thread.joinable())
        {
            _rpc_recv_thread.join();
        }
#endif
    // LOG(INFO) << "Session shutdown.";
}

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_STATESUBSCRIBERGRPC_H__