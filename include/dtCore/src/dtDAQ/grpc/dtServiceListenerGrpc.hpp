// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSERVICELISTENERGRPC_H__
#define __DTCORE_DTSERVICELISTENERGRPC_H__

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

#include <dtCore/src/dtDAQ/dtDataSink.h>
#include <dtCore/dtLog>

#include <dtProto/Service.grpc.pb.h>

namespace dtCore {

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// dtServiceListenerGrpc Declaration
//
class dtServiceListenerGrpc : public dtDataSink
{ 
public:
    dtServiceListenerGrpc(const std::string& server_address);
    ~dtServiceListenerGrpc();
    // virtual void Publish() {}

protected:
public:
    void Run();
    void Stop();
    bool IsRun();

public:
    template<typename SesstionType> bool AddSession();
    void RemoveSession(uint64_t session_id);

public:
    class Session {
    public:
        Session(dtServiceListenerGrpc* server, dtproto::dtService::AsyncService* service, grpc::ServerCompletionQueue* cq);
        Session() = delete;
        virtual ~Session() = default;
        virtual void OnCompletionEvent() = 0;
        uint64_t GetId();
        void TryCancelCallAndShutdown();

    protected:
        uint64_t _id;
        dtServiceListenerGrpc* _server;
        dtproto::dtService::AsyncService* _service;
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

    public:
        static uint64_t AllocSessionId();
    };

    friend class Session;

protected:
    std::string _server_address;
    std::unique_ptr<grpc::Server> _server;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    dtproto::dtService::AsyncService _service;
    std::atomic<bool> _running {false};
    std::thread _rpc_thread;    
    std::mutex _session_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<Session> > _sessions;
};

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// dtServiceListenerGrpc::Session Implementation
//
uint64_t dtServiceListenerGrpc::Session::AllocSessionId()
{
    static std::atomic<uint64_t> _session_id_allocator{0};
    return (++_session_id_allocator);
}

dtServiceListenerGrpc::Session::Session(dtServiceListenerGrpc* server, dtproto::dtService::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : _server(server), _service(service), _cq(cq), _status(SessionStatus::WAIT_CONNECT)
{    
    _id = AllocSessionId();
    _status = SessionStatus::WAIT_CONNECT;
}

uint64_t dtServiceListenerGrpc::Session::GetId() 
{ 
    return _id; 
}

void dtServiceListenerGrpc::Session::TryCancelCallAndShutdown()
{
    //std::lock_guard<std::mutex> lock(_proc_mtx);
    if (_status != SessionStatus::WAIT_CONNECT &&
        _status != SessionStatus::WAIT_FINISH &&
        _status != SessionStatus::FINISHED) {
        _ctx.TryCancel();

        // LOG(INFO) << "Finishing<" << _id << ">.";
        std::lock_guard<std::mutex> lock(_proc_mtx);
        //_status = SessionStatus::WAIT_FINISH;
        _status = SessionStatus::FINISHED;
    }

    // LOG(INFO) << "Session shutdown.";
    // _status = SessionStatus::FINISHED;
    // _server->RemoveSession(_id);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// dtServiceListenerGrpc Implementation
//
dtServiceListenerGrpc::dtServiceListenerGrpc(const std::string& server_address)
: _server_address(server_address)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(_server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&_service);
    _cq = builder.AddCompletionQueue();
    _server = builder.BuildAndStart();
    // AddSession<OnControlCmd>();
    Run();
}

dtServiceListenerGrpc::~dtServiceListenerGrpc()
{
    Stop();
}

template<typename SessionType>
bool dtServiceListenerGrpc::AddSession() {
    std::shared_ptr<SessionType> session =  std::make_shared<SessionType>(this, &_service, _cq.get());
    std::lock_guard<std::mutex> lock(_session_mtx);
    _sessions[session->GetId()] = session;
    return true;
}

void dtServiceListenerGrpc::RemoveSession(uint64_t session_id) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    _sessions.erase(session_id);
}

// Stop all pending rpc calls and close sessions
void dtServiceListenerGrpc::Stop() {
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
    _rpc_thread.join();
    // LOG(INFO) << "Server shutdown.";
}

// Run grpc message-dispatcher
void dtServiceListenerGrpc::Run() {
    _running = true;

    // rpc event "read done / write done / close(already connected)" call-back by this call completion queue
    // rpc event "new connection / close(waiting for connect)" call-back by this notification completion queue
    _rpc_thread = std::thread([this] {
        // LOG(INFO) << "RPC new-call handler()";
        void* tag;
        bool ok;
        while (_cq->Next(&tag, &ok)) {
            // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
            //GPR_ASSERT(ok);
            if (ok) {
                static_cast<dtServiceListenerGrpc::Session*>(tag)->OnCompletionEvent();
            }
            else {
                static_cast<dtServiceListenerGrpc::Session*>(tag)->TryCancelCallAndShutdown();
            }
        }
    });
}

bool dtServiceListenerGrpc::IsRun() {
    return _running.load();
}


/////////////////////////////////////////////////////////////////////////////////////////////////
//
// OnControlCmd
//
class OnControlCmd : public dtServiceListenerGrpc::Session {
public:
    OnControlCmd(dtServiceListenerGrpc* server, dtproto::dtService::AsyncService* service, grpc::ServerCompletionQueue* cq)
    : Session(server, service, cq), _responder(&_ctx) {
        //LOG(INFO) << "NEW OnControlCmd session created.";
        _status = SessionStatus::WAIT_CONNECT;
        _service->RequestCommand(&_ctx, &_request, &_responder, _cq, _cq, this);
        //LOG(INFO) << "Wait for new ControlService::Command() service call...";
    }
    ~OnControlCmd() {
        //LOG(INFO) << "OnControlCmd session deleted.";
    }
    void OnCompletionEvent() {
        //LOG(INFO) << "OnControlCmd: " << "OnCompletionEvent";
        if (_status == SessionStatus::WAIT_CONNECT) {
            //LOG(INFO) << "NEW ControlService::Command() service call.";

            _server->AddSession<OnControlCmd>();

            {
                std::lock_guard<std::mutex> lock(_proc_mtx);

                _response.set_rtn(0);
                _response.set_msg("success");

                _status = SessionStatus::WAIT_FINISH;
                _responder.Finish(_response, grpc::Status::OK, this);
            }
        } 
        else if (_status == SessionStatus::WAIT_FINISH) {
            //LOG(INFO) << "Finalize ControlService::Command() service.";
            //_status = SessionStatus::FINISHED;
            _server->RemoveSession(_id);
        }
        else {
            GPR_ASSERT(false && "Invalid Session Status.");
            //LOG(ERROR) << "Invalid session status (" << static_cast<int>(_status) << ")";
        }
    }

private:
    ::dtproto::robot_msgs::ControlCmd _request;
    ::dtproto::std_msgs::Response _response;
    grpc::ServerAsyncResponseWriter<::dtproto::std_msgs::Response> _responder;
};


}

#endif // __DTCORE_DTSERVICELISTENERGRPC_H__