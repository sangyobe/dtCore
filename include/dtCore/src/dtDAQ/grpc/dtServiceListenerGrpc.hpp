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

#define USE_THREAD_PTHREAD

namespace dtCore {

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// dtServiceListenerGrpc Declaration
//
class dtServiceListenerGrpc : public dtDataSink
{ 
public:
  dtServiceListenerGrpc(std::unique_ptr<grpc::Service> service,
                        const std::string &server_address)
      : _service(std::move(service)), _server_address(server_address) {
    grpc::ServerBuilder builder;
    builder.AddListeningPort(_server_address,
                             grpc::InsecureServerCredentials());
    builder.RegisterService(_service.get());
    _cq = builder.AddCompletionQueue();
    _server = builder.BuildAndStart();
    Run();
  }

    ~dtServiceListenerGrpc() { Stop(); }
    // virtual void Publish() {}

  protected:
  public:
    // Run grpc message-dispatcher
  void Run() {
    _running = true;

    // rpc event "read done / write done / close(already connected)" call-back
    // by this call completion queue rpc event "new connection / close(waiting
    // for connect)" call-back by this notification completion queue
#ifdef USE_THREAD_PTHREAD
        pthread_create(
            &_rpc_thread, NULL,
            [](void *arg) -> void * {
              dtServiceListenerGrpc *client = (dtServiceListenerGrpc *)arg;

              void *tag;
              bool ok;
              while (client->_cq->Next(&tag, &ok)) {
                // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
                // GPR_ASSERT(ok);
                if (ok) {
                  static_cast<dtServiceListenerGrpc::Session *>(tag)
                      ->OnCompletionEvent();
                } else {
                  static_cast<dtServiceListenerGrpc::Session *>(tag)
                      ->TryCancelCallAndShutdown();
                }
              }
              return 0;
            },
            (void *)this);
#else
        _rpc_thread = std::thread([this] {
          void *tag;
          bool ok;
          while (_cq->Next(&tag, &ok)) {
            // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
            // GPR_ASSERT(ok);
            if (ok) {
              static_cast<dtServiceListenerGrpc::Session *>(tag)
                  ->OnCompletionEvent();
            } else {
              static_cast<dtServiceListenerGrpc::Session *>(tag)
                  ->TryCancelCallAndShutdown();
            }
          }
        });
#endif
  }

    // Stop all pending rpc calls and close sessions
    void Stop() {
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
#ifdef USE_THREAD_PTHREAD
      void *th_join_result;
      pthread_join(_rpc_thread, &th_join_result);
#else
      _rpc_thread.join();
#endif
    }
    bool IsRun() { return _running.load(); }

  public:
    template <typename SessionType> bool AddSession(void *udata = nullptr) {
      std::shared_ptr<SessionType> session =
          std::make_shared<SessionType>(this, _service.get(), _cq.get(), udata);
      std::lock_guard<std::mutex> lock(_session_mtx);
      _sessions[session->GetId()] = session;
      return true;
    }

    void RemoveSession(uint64_t session_id) {
      std::lock_guard<std::mutex> lock(_session_mtx);
      _sessions.erase(session_id);
    }

  public:
    class Session {
    public:
      Session(dtServiceListenerGrpc *server, grpc::Service *service,
              grpc::ServerCompletionQueue *cq,
              void * /*placeholder for user data*/)
          : _server(server), _service(service), _cq(cq),
            _call_state(CallState::WAIT_CONNECT) {
        _id = AllocSessionId();
        _call_state = CallState::WAIT_CONNECT;
      }

      Session() = delete;
      virtual ~Session() = default;
      virtual void OnCompletionEvent() = 0;

      uint64_t GetId() { return _id; }

      void TryCancelCallAndShutdown() {
        // std::lock_guard<std::mutex> lock(_proc_mtx);
        if (_call_state != CallState::WAIT_CONNECT &&
            _call_state != CallState::WAIT_FINISH &&
            _call_state != CallState::FINISHED) {
          _ctx.TryCancel();

          // LOG(INFO) << "Finishing<" << _id << ">.";
          std::lock_guard<std::mutex> lock(_proc_mtx);
          //_call_state = CallState::WAIT_FINISH;
          _call_state = CallState::FINISHED;
        }

        // LOG(INFO) << "Session shutdown.";
        // _call_state = CallState::FINISHED;
        // _server->RemoveSession(_id);
      }

    protected:
      uint64_t _id;
      dtServiceListenerGrpc *_server;
      grpc::Service *_service;
      grpc::ServerCompletionQueue *_cq;
      grpc::ServerContext _ctx;
      std::mutex _proc_mtx;

      enum class CallState {
        WAIT_CONNECT,
        READY_TO_READ,
        WAIT_READ_DONE,
        READY_TO_WRITE,
        WAIT_WRITE_DONE,
        WAIT_FINISH,
        FINISHED,
        PEER_DISCONNECTED
      };
      CallState _call_state;

    public:
      static uint64_t AllocSessionId() {
        static std::atomic<uint64_t> _session_id_allocator{0};
        return (++_session_id_allocator);
      }
    };

    friend class Session;

  protected:
    std::string _server_address;
    std::unique_ptr<grpc::Server> _server;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    std::unique_ptr<grpc::Service> _service;
    std::atomic<bool> _running{false};
#ifdef USE_THREAD_PTHREAD
    pthread_t _rpc_thread;
#else
    std::thread _rpc_thread;
#endif
    std::mutex _session_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<Session>> _sessions;
};

} // namespace dtCore

#endif // __DTCORE_DTSERVICELISTENERGRPC_H__