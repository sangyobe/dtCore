// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSTATEPUBLISHERGRPC_H__
#define __DTCORE_DTSTATEPUBLISHERGRPC_H__

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

#include "dtProto/Service.grpc.pb.h"

#include "../dtDataSinkPB.hpp"

namespace dtCore {

template<typename StateType>
class dtStatePublisherGrpc : public dtDataSinkPB<StateType>
{ 
public:
    dtStatePublisherGrpc(const std::string& topic_name, const std::string& server_address);
    ~dtStatePublisherGrpc();

    void Publish(StateType& msg);

protected:
    void Run();
    void Stop();
    bool IsRun();

    bool AddSession();
    void RemoveSession(uint64_t session_id);

    class Session {
    public:
        Session(dtStatePublisherGrpc<StateType>* server, dtproto::service::DataService::AsyncService* service, grpc::ServerCompletionQueue* cq);
        Session() = delete;
        virtual ~Session();
        virtual void OnCompletionEvent();
        virtual void Publish(StateType& msg);
        uint64_t GetId();
        void TryCancelCallAndShutdown();

    protected:
        uint64_t _id;
        dtStatePublisherGrpc<StateType>* _server;
        dtproto::service::DataService::AsyncService* _service;
        grpc::ServerCompletionQueue* _cq;
        grpc::ServerContext _ctx;
        std::mutex _proc_mtx;

        enum class SessionStatus {
            WAIT_CONNECT,
            READY_TO_WRITE,
            WAIT_WRITE_DONE,
            WAIT_FINISH,
            FINISHED,
            PEER_DISCONNECTED
        };
        SessionStatus _status;

        dtproto::std_msgs::Request _request;
        grpc::ServerAsyncWriter<dtproto::std_msgs::State> _responder;
        uint32_t _msg_seq{0};
        dtproto::std_msgs::State _msg;
        std::deque<dtproto::std_msgs::State> _msg_queue{};

    public:
        static uint64_t AllocSessionId();
    };


    friend class Session;

protected:
    std::string _server_address;
    std::string _topic_name;
    std::unique_ptr<grpc::Server> _server;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    dtproto::service::DataService::AsyncService _service;
    std::atomic<bool> _running {false};
    std::thread _rpc_thread;    
    std::mutex _session_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<Session> > _sessions;
};

template<typename StateType>
uint64_t dtStatePublisherGrpc<StateType>::Session::AllocSessionId()
{
    static std::atomic<uint64_t> _session_id_allocator{0};
    return (++_session_id_allocator);
}

template<typename StateType>
dtStatePublisherGrpc<StateType>::Session::Session(dtStatePublisherGrpc<StateType>* server, dtproto::service::DataService::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : _server(server), _service(service), _cq(cq), _status(SessionStatus::WAIT_CONNECT), _responder(&_ctx) 
{    
    _id = AllocSessionId();
    // LOG(INFO) << "Session id: " << _id;

    // LOG(INFO) << "NEW StreamStateSession created.";
    _service->RequestStreamState(&_ctx, &_request, &_responder, _cq, _cq, this);
    _status = SessionStatus::WAIT_CONNECT;
    // LOG(INFO) << "Wait for new StreamState() service call...";
}

template<typename StateType>
dtStatePublisherGrpc<StateType>::Session::~Session()
{
    // LOG(INFO) << "StreamStateSession deleted.";
}

template<typename StateType>
void dtStatePublisherGrpc<StateType>::Session::OnCompletionEvent()
{
    // LOG(INFO) << "StreamState: " << "OnCompletionEvent";
    if (_status == SessionStatus::WAIT_CONNECT) {
        // LOG(INFO) << "NEW StreamState() service call.";

        _server->AddSession();

        {
            std::lock_guard<std::mutex> lock(_proc_mtx);
            _status = SessionStatus::READY_TO_WRITE;
        }
      
    } 
    else if (_status == SessionStatus::WAIT_WRITE_DONE) {

      std::lock_guard<std::mutex> lock(_proc_mtx);

      if (!_msg_queue.empty()) {
        _status = SessionStatus::WAIT_WRITE_DONE;
        _responder.Write(_msg_queue.front(), this);
        _msg_queue.pop_front();
      }
      else {
        _status = SessionStatus::READY_TO_WRITE;
      }
    }
    else if (_status == SessionStatus::WAIT_FINISH) {
      // LOG(INFO) << "Finalize StreamState() service.";
      //_status = SessionStatus::FINISHED;
      _server->RemoveSession(_id);
    }
    else {
      GPR_ASSERT(false && "Invalid Session Status.");
      // LOG(ERROR) << "Invalid session status (" << static_cast<int>(_status) << ")";
    }
}

template<typename StateType>
void dtStatePublisherGrpc<StateType>::Session::Publish(StateType& msg)
{
    std::lock_guard<std::mutex> lock(_proc_mtx);
    
    if (_status == SessionStatus::WAIT_WRITE_DONE) {
        // LOG(INFO) << "StreamStateSession<" << _id << ">::Queue(" << _msg_seq << ")";
        _msg.mutable_state()->PackFrom(msg);
        _msg_queue.push_back(_msg);
    }
    else if (_status == SessionStatus::READY_TO_WRITE) {
        // LOG(INFO) << "StreamStateSession<" << _id << ">::Write(" << _msg_seq << ")";
        _msg.mutable_state()->PackFrom(msg);
        _status = SessionStatus::WAIT_WRITE_DONE;
        _responder.Write(_msg, this);
    }
}

template<typename StateType>
uint64_t dtStatePublisherGrpc<StateType>::Session::GetId() 
{ 
    return _id; 
}

template<typename StateType>
void dtStatePublisherGrpc<StateType>::Session::TryCancelCallAndShutdown()
{
    //std::lock_guard<std::mutex> lock(_proc_mtx);
    if (_status != SessionStatus::WAIT_CONNECT)
        _ctx.TryCancel();
    // LOG(INFO) << "Session shutdown.";
    _status = SessionStatus::FINISHED;
    _server->RemoveSession(_id);
}



template<typename StateType>
dtStatePublisherGrpc<StateType>::dtStatePublisherGrpc(const std::string& topic_name, const std::string& server_address)
: _topic_name(topic_name), _server_address(server_address)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&_service);
    _cq = builder.AddCompletionQueue();
    _server = builder.BuildAndStart();

    AddSession();
    Run();
}

template<typename StateType>
dtStatePublisherGrpc<StateType>::~dtStatePublisherGrpc()
{
    Stop();
}

template<typename StateType>
void dtStatePublisherGrpc<StateType>::Publish(StateType& msg) 
{
    std::lock_guard<std::mutex> lock(_session_mtx);
    for (auto it : _sessions) {
      it.second->Publish(msg);
    }
}

template<typename StateType>
bool dtStatePublisherGrpc<StateType>::AddSession() {
    std::shared_ptr<dtStatePublisherGrpc<StateType>::Session> session =  std::make_shared<dtStatePublisherGrpc<StateType>::Session>(this, &_service, _cq.get());
    std::lock_guard<std::mutex> lock(_session_mtx);
    _sessions[session->GetId()] = session;
    return true;
}

template<typename StateType>
void dtStatePublisherGrpc<StateType>::RemoveSession(uint64_t session_id) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    _sessions.erase(session_id);
}

// Stop all pending rpc calls and close sessions
template<typename StateType>
void dtStatePublisherGrpc<StateType>::Stop() {
    // LOG(INFO) << "Shutting down Server.";

    std::lock_guard<std::mutex> lock(_session_mtx);
    for (auto it : _sessions) {
        it.second->TryCancelCallAndShutdown();
    }

    _server->Shutdown();
    _cq->Shutdown();
    _running = false;
    _rpc_thread.join();
    // LOG(INFO) << "Server shutdown.";
}

// Run grpc message-dispatcher
template<typename StateType>
void dtStatePublisherGrpc<StateType>::Run() {
    _running = true;

    // rpc event "read done / write done / close(already connected)" call-back by this call completion queue
    // rpc event "new connection / close(waiting for connect)" call-back by this notification completion queue
    _rpc_thread = std::thread([this] {
        // LOG(INFO) << "RPC new-call handler()";
        void* tag;
        bool ok;
        while (_cq->Next(&tag, &ok)) {
            // LOG(INFO) << "CQ_CALL";
            //GPR_ASSERT(ok);
            if (ok) {
                static_cast<dtStatePublisherGrpc<StateType>::Session*>(tag)->OnCompletionEvent();
            }
            else {
                static_cast<dtStatePublisherGrpc<StateType>::Session*>(tag)->TryCancelCallAndShutdown();
            }
        }
    });
}

template<typename StateType>
bool dtStatePublisherGrpc<StateType>::IsRun() {
    return _running.load();
}


}

#endif // __DTCORE_DTSTATEPUBLISHERGRPC_H__