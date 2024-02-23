// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSERVICECALLERGRPC_H__
#define __DTCORE_DTSERVICECALLERGRPC_H__

/** \defgroup dtDAQ
 *
 */
#include <grpcpp/grpcpp.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include <dtProto/Service.grpc.pb.h>

#define USE_THREAD_PTHREAD

namespace dtCore {

/////////////////////////////////////////////////////////////////////////////////////////////////
//
// dtServiceCallerGrpc Declaration
//
template <typename ServiceType> class dtServiceCallerGrpc {
public:
  dtServiceCallerGrpc(const std::string &server_address)
      : _stub(ServiceType::NewStub(grpc::CreateChannel(
            server_address, grpc::InsecureChannelCredentials()))) {
    Run();
  }

  ~dtServiceCallerGrpc() { Stop(); }

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
          dtServiceCallerGrpc<ServiceType> *client =
              (dtServiceCallerGrpc<ServiceType> *)arg;

          void *tag;
          bool ok;
          while (client->_cq.Next(&tag, &ok)) {
            // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
            // GPR_ASSERT(ok);
            if (tag) {
              if (!static_cast<dtServiceCallerGrpc<ServiceType>::Call *>(tag)->OnCompletionEvent(ok)) {
                // static_cast<dtServiceCallerGrpc<ServiceType>::Call*>(tag)->TryCancelCallAndShutdown();
                client->RemoveCall(static_cast<dtServiceCallerGrpc<ServiceType>::Call *>(tag)->GetId());
              }
            }
          }
          return 0;
        },
        (void *)this);
#else
    _rpc_thread = std::thread([this] {
      void *tag;
      bool ok;
      while (_cq.Next(&tag, &ok)) {
        // LOG(INFO) << "CQ_CALL(" << (ok ? "true" : "false") << ")";
        // GPR_ASSERT(ok);
        if (tag) {
          if (!static_cast<dtServiceCallerGrpc<ServiceType>::Call *>(tag)->OnCompletionEvent(ok)) {
            // static_cast<dtServiceCallerGrpc<ServiceType>::Call*>(tag)->TryCancelCallAndShutdown();
            RemoveCall(static_cast<dtServiceCallerGrpc<ServiceType>::Call *>(tag)->GetId());
        }
      }
    });
#endif
  }

  // Stop all pending rpc calls and close calls
  void Stop() {
    // {
    //   std::lock_guard<std::mutex> lock(_call_list_mtx);
    //   for (auto it : _calls) {
    //     it.second->TryCancelCallAndShutdown();
    //   }
    //   //_calls.clear();
    // }

    _cq.Shutdown();

#ifdef USE_THREAD_PTHREAD
    void *th_join_result;
    pthread_join(_rpc_thread, &th_join_result);
#else
    _rpc_thread.join();
#endif

    // drain the queue
    void *ignoredTag = nullptr;
    bool ok = false;
    while (_cq.Next(&ignoredTag, &ok)) {}

    _running = false;
  }

  bool IsRun() { return _running.load(); }

public:
  template <typename CallType> bool StartCall(void *udata = nullptr) {
    std::shared_ptr<CallType> call =
        std::make_shared<CallType>(_stub.get(), &_cq, udata);
    std::lock_guard<std::mutex> lock(_call_list_mtx);
    _calls[call->GetId()] = call;
    return true;
  }

  void RemoveCall(uint64_t call_id) {
    std::lock_guard<std::mutex> lock(_call_list_mtx);
    _calls.erase(call_id);
  }

public:
  class Call {
  public:
    Call(typename ServiceType::Stub *stub, grpc::CompletionQueue *cq,
         void * /*placeholder for user data*/)
        : _stub(stub), _cq(cq) {
      _id = AllocCallId();
      _call_state = CallState::WAIT_CONNECT;
    }

    Call() = delete;
    virtual ~Call() = default;
    virtual bool OnCompletionEvent(bool ok) = 0;

    uint64_t GetId() { return _id; }

    bool TryCancelCallAndShutdown() {
        std::lock_guard<std::mutex> lock(_proc_mtx);
        _ctx.TryCancel();
        return true;
    }

  protected:
    uint64_t _id;
    typename ServiceType::Stub *_stub;
    grpc::CompletionQueue *_cq;
    grpc::ClientContext _ctx;
    grpc::Status _status;
    std::mutex _proc_mtx;

    enum class CallState {
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
    CallState _call_state;

  public:
    static uint64_t AllocCallId() {
      static std::atomic<uint64_t> _call_id_allocator{0};
      return (++_call_id_allocator);
    }
  };
  friend class Call;

protected:
  grpc::CompletionQueue _cq;
  std::unique_ptr<typename ServiceType::Stub> _stub;
  std::atomic<bool> _running{false};
#ifdef USE_THREAD_PTHREAD
  pthread_t _rpc_thread;
#else
  std::thread _rpc_thread;
#endif
  std::mutex _call_list_mtx;
  std::unordered_map<uint64_t, std::shared_ptr<Call>> _calls;
};

} // namespace dtCore

#endif // __DTCORE_DTSERVICECALLERGRPC_H__