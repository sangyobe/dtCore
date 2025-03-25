// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_STATEPUBLISHERGRPC_H__
#define __DT_DAQ_STATEPUBLISHERGRPC_H__

/** \defgroup dtDAQ
 *
 */
#include <grpc/grpc.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <deque>
#include <pthread.h>

#include "dtProto/Service.grpc.pb.h"

#include "../dtDataSinkPB.hpp"

namespace dt
{
namespace DAQ
{

template <typename StateType>
class StatePublisherGrpc : public DataSinkPB<StateType>
{ 
public:
    StatePublisherGrpc(const std::string &topic_name, const std::string &server_address, int queue_size = -1);
    ~StatePublisherGrpc();

    void Publish(StateType& msg);

protected:
    void Run();
    void Stop();
    bool IsRun();

    bool AddSession();
    void RemoveSession(uint64_t session_id);

    class Session {
    public:
        Session(StatePublisherGrpc<StateType> *server, dtproto::dtService::AsyncService *service, grpc::ServerCompletionQueue *cq);
        Session() = delete;
        virtual ~Session();
        virtual void OnCompletionEvent();
        virtual void Publish(StateType& msg);
        uint64_t GetId();
        void TryCancelCallAndShutdown();

    protected:
        uint64_t _id;
        StatePublisherGrpc<StateType> *_server;
        dtproto::dtService::AsyncService* _service;
        grpc::ServerCompletionQueue* _cq;
        grpc::ServerContext _ctx;
        std::mutex _proc_mtx;

        enum class CallState {
            WAIT_CONNECT,
            READY_TO_WRITE,
            WAIT_WRITE_DONE,
            WAIT_FINISH,
            FINISHED,
            PEER_DISCONNECTED
        };
        CallState _call_state;

        dtproto::std_msgs::Request _request;
        grpc::ServerAsyncWriter<dtproto::std_msgs::State> _responder;
        grpc::Status _finish_status = grpc::Status::OK;
        uint32_t _msg_seq{0};
        dtproto::std_msgs::State _msg;
        std::deque<dtproto::std_msgs::State> _msg_queue{};

    public:
        static uint64_t AllocSessionId();
    };


    friend class Session;

protected:
    std::string _topic_name;
    std::string _server_address;
    std::unique_ptr<grpc::Server> _server;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    dtproto::dtService::AsyncService _service;
    std::atomic<bool> _running {false};
#if defined(_WIN32) || defined(__CYGWIN__)
    std::thread _rpc_thread;
#else
    pthread_t _rpc_thread;
#endif    
    std::mutex _session_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<Session> > _sessions;
    int _msg_queue_size;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of StatePublisherGrpc::Session
//
template <typename StateType>
uint64_t StatePublisherGrpc<StateType>::Session::AllocSessionId()
{
    static std::atomic<uint64_t> _session_id_allocator{0};
    return (++_session_id_allocator);
}

template <typename StateType>
StatePublisherGrpc<StateType>::Session::Session(StatePublisherGrpc<StateType> *server, dtproto::dtService::AsyncService *service, grpc::ServerCompletionQueue *cq)
    : _server(server), _service(service), _cq(cq), _call_state(CallState::WAIT_CONNECT), _responder(&_ctx)
{    
    _id = AllocSessionId();
    // LOG(INFO) << "Session id: " << _id;

    // initialize mutex attributes
    int rtn;
    pthread_mutexattr_t mtx_attr;
    rtn = pthread_mutexattr_init(&mtx_attr);
    if (rtn) printf("error: pthread_mutexattr_init (%d)\n\n", rtn);
    
    rtn = pthread_mutexattr_setprotocol(&mtx_attr, PTHREAD_PRIO_NONE);
    if (rtn) printf("error: pthread_mutexattr_setprotocol (%d)\n\n", rtn);

    rtn = pthread_mutexattr_setpshared(&mtx_attr, PTHREAD_PROCESS_SHARED);
    if (rtn) printf("error: pthread_mutexattr_setpshared (%d)\n\n", rtn);

    rtn = pthread_mutexattr_settype(&mtx_attr, PTHREAD_MUTEX_RECURSIVE);
    if (rtn) printf("error: pthread_mutexattr_settype (%d)\n\n", rtn);
    
    rtn = pthread_mutexattr_setprioceiling(&mtx_attr, 99);
    if (rtn) printf("error: pthread_mutexattr_setprioceiling (%d)\n\n", rtn);
    
    rtn = pthread_mutex_init(_proc_mtx.native_handle(), &mtx_attr);
    if (rtn) printf("error: pthread_mutex_init (%d)\n\n", rtn);

    _service->RequestPublishState(&_ctx, &_request, &_responder, _cq, _cq, this);
    _call_state = CallState::WAIT_CONNECT;
    // LOG(INFO) << "Wait for new PublishState() service call...";
}

template <typename StateType>
StatePublisherGrpc<StateType>::Session::~Session()
{
    // LOG(INFO) << "StreamStateSession deleted.";
}

template <typename StateType>
void StatePublisherGrpc<StateType>::Session::OnCompletionEvent()
{
    if (_call_state == CallState::WAIT_CONNECT) {
        // LOG(INFO) << "NEW PublishState() service call.";
        _server->AddSession();
        {
            std::lock_guard<std::mutex> lock(_proc_mtx);
            _call_state = CallState::READY_TO_WRITE;
        }
    } 
    else if (_call_state == CallState::WAIT_WRITE_DONE) {
        // LOG(INFO) << "Write done.";
        std::lock_guard<std::mutex> lock(_proc_mtx);
        if (!_msg_queue.empty()) {
            _call_state = CallState::WAIT_WRITE_DONE;
            _responder.Write(_msg_queue.front(), this);
            _msg_queue.pop_front();
        }
        else {
            _call_state = CallState::READY_TO_WRITE;
        }
    }
    else if (_call_state == CallState::WAIT_FINISH) {
        // LOG(INFO) << "Finalize PublishState() service.";
        //_call_state = CallState::FINISHED;
        _server->RemoveSession(_id);
    }
    else {
        // LOG(ERROR) << "Invalid session status (" << static_cast<int>(_call_state) << ")";
        // GPR_ASSERT(false && "Invalid Session Status.");
    }
}

template <typename StateType>
void StatePublisherGrpc<StateType>::Session::Publish(StateType &msg)
{
    std::lock_guard<std::mutex> lock(_proc_mtx);
    
    if (_call_state == CallState::WAIT_WRITE_DONE) {
        // LOG(INFO) << "StreamStateSession<" << _id << ">::Queue(" << _msg_seq << ")";
        if (_server->_msg_queue_size == 0) {
            return; // delete this message !
        }
        else if (_server->_msg_queue_size > 0 && _msg_queue.size() >= (uint32_t)(_server->_msg_queue_size)) {
            _msg_queue.pop_front(); // delete the oldest message.
        }
        _msg.mutable_state()->PackFrom(msg);
        _msg_queue.push_back(_msg);
    }
    else if (_call_state == CallState::READY_TO_WRITE) {
        // LOG(INFO) << "StreamStateSession<" << _id << ">::Write(" << _msg_seq << ")";
        _msg.mutable_state()->PackFrom(msg);
        _call_state = CallState::WAIT_WRITE_DONE;
        _responder.Write(_msg, this);
    }
}

template <typename StateType>
uint64_t StatePublisherGrpc<StateType>::Session::GetId()
{ 
    return _id; 
}

template <typename StateType>
void StatePublisherGrpc<StateType>::Session::TryCancelCallAndShutdown()
{
    //std::lock_guard<std::mutex> lock(_proc_mtx);
    if (_call_state != CallState::WAIT_CONNECT &&
        _call_state != CallState::WAIT_FINISH &&
        _call_state != CallState::FINISHED) {
        _ctx.TryCancel();

        // LOG(INFO) << "Finishing<" << _id << ">.";
        std::lock_guard<std::mutex> lock(_proc_mtx);
        //_call_state = CallState::WAIT_FINISH;
        _call_state = CallState::FINISHED;
        //_responder.Finish(_finish_status, this);
    }

    // LOG(INFO) << "Session shutdown.";
    // _call_state = CallState::FINISHED;
    // _server->RemoveSession(_id);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Implementation of StatePublisherGrpc
//
template <typename StateType>
StatePublisherGrpc<StateType>::StatePublisherGrpc(const std::string &topic_name, const std::string &server_address, int queue_size)
    : _topic_name(topic_name), _server_address(server_address), _msg_queue_size(queue_size)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(_server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&_service);
    _cq = builder.AddCompletionQueue();
    _server = builder.BuildAndStart();

    AddSession();
    Run();
}

template <typename StateType>
StatePublisherGrpc<StateType>::~StatePublisherGrpc()
{
    Stop();
}

template <typename StateType>
void StatePublisherGrpc<StateType>::Publish(StateType &msg)
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    for (auto it : _sessions) {
        it.second->Publish(msg);
    }
}

template <typename StateType>
bool StatePublisherGrpc<StateType>::AddSession()
{
    std::shared_ptr<StatePublisherGrpc<StateType>::Session> session = std::make_shared<StatePublisherGrpc<StateType>::Session>(this, &_service, _cq.get());
    std::lock_guard<std::mutex> lock(_session_mtx);
    _sessions[session->GetId()] = session;
    return true;
}

template <typename StateType>
void StatePublisherGrpc<StateType>::RemoveSession(uint64_t session_id)
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    _sessions.erase(session_id);
}

// Stop all pending rpc calls and close sessions
template <typename StateType>
void StatePublisherGrpc<StateType>::Stop()
{
    // LOG(INFO) << "Shutting down Server.";

    {
        std::lock_guard<std::mutex> lock(_session_mtx);
        for (auto it : _sessions) {
            it.second->TryCancelCallAndShutdown();
        }
        //_sessions.clear();
    }

    _running = false;
    _server->Shutdown();
    _cq->Shutdown();
#if defined(_WIN32) || defined(__CYGWIN__)
    if (_rpc_thread.joinable())
    {
        _rpc_thread.join();
    }
#else
    if (_rpc_thread)
    {
        pthread_join(_rpc_thread, nullptr);
        _rpc_thread = (pthread_t)nullptr;
    }
#endif
    // LOG(INFO) << "Server shutdown.";
}

// Run grpc message-dispatcher
template <typename StateType>
void StatePublisherGrpc<StateType>::Run()
{
    _running = true;

    // rpc event "read done / write done / close(already connected)" call-back by this call completion queue
    // rpc event "new connection / close(waiting for connect)" call-back by this notification completion queue
#if defined(_WIN32) || defined(__CYGWIN__)
    _rpc_thread = std::thread([this] {
        // LOG(INFO) << "RPC new-call handler()";
        void *tag;
        bool ok;
        while (_cq->Next(&tag, &ok))
        {
            // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
            //GPR_ASSERT(ok);
            if (ok)
            {
                static_cast<StatePublisherGrpc<StateType>::Session *>(tag)->OnCompletionEvent();
            }
            else
            {
                static_cast<StatePublisherGrpc<StateType>::Session *>(tag)->TryCancelCallAndShutdown();
            }
        }
    });
#else
    pthread_create(
        &_rpc_thread, NULL,
        [](void *arg) -> void * {
            StatePublisherGrpc<StateType> *server = (StatePublisherGrpc<StateType> *)arg;

            // LOG(INFO) << "RPC new-call handler()";
            void *tag;
            bool ok;
            while (server->_cq->Next(&tag, &ok))
            {
                // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
                //GPR_ASSERT(ok);
                if (ok)
                {
                    static_cast<StatePublisherGrpc<StateType>::Session *>(tag)->OnCompletionEvent();
                }
                else
                {
                    static_cast<StatePublisherGrpc<StateType>::Session *>(tag)->TryCancelCallAndShutdown();
                }
            }
            return 0;
        },
        (void *)this);
#endif
}

template <typename StateType>
bool StatePublisherGrpc<StateType>::IsRun()
{
    return _running.load();
}

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_STATEPUBLISHERGRPC_H__