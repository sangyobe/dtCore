// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_SERVICECALLERGRPC_H__
#define __DT_DAQ_SERVICECALLERGRPC_H__

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

namespace dt
{
namespace DAQ
{

/*!
 * @brief ServiceCallerGrpc class.
 * @details Is serves as the main interface for RPC service calls.
 * It owns and runs RPC event message dispatcher thread.
 * To initiate a new RPC call, call StartCall() method with proper subclass of ServiceCallerGrpc::Call template parameter.
 */
template <typename ServiceType> class ServiceCallerGrpc
{
public:
    ServiceCallerGrpc(const std::string &server_address)
        : _stub(ServiceType::NewStub(grpc::CreateChannel(
              server_address, grpc::InsecureChannelCredentials())))
    {
        Run();
    }

    ~ServiceCallerGrpc() { Stop(); }

public:
    /*!
     * Run grpc message-dispatcher.
     * It launches a thread to check grpc I/O completion.
     * User does not need to call Run() explicitly. dtServiceCallGrpc() constructor will do.
     */
    void Run()
    {
        _running = true;

        // rpc event "read done / write done / close(already connected)" call-back
        // by this call completion queue rpc event "new connection / close(waiting
        // for connect)" call-back by this notification completion queue
#ifdef USE_THREAD_PTHREAD
        pthread_create(
            &_rpc_thread, NULL,
            [](void *arg) -> void * {
                ServiceCallerGrpc<ServiceType> *client =
                    (ServiceCallerGrpc<ServiceType> *)arg;

                void *tag;
                bool ok;
                while (client->_cq.Next(&tag, &ok))
                {
                    // GPR_ASSERT(ok);
                    if (tag)
                    {
                        if (!static_cast<ServiceCallerGrpc<ServiceType>::Call *>(tag)->OnCompletionEvent(ok))
                        {
                            // static_cast<ServiceCallerGrpc<ServiceType>::Call*>(tag)->TryCancelCallAndShutdown();
                            client->RemoveCall(static_cast<ServiceCallerGrpc<ServiceType>::Call *>(tag)->GetId());
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
            while (_cq.Next(&tag, &ok))
            {
                // GPR_ASSERT(ok);
                if (tag)
                {
                    if (!static_cast<ServiceCallerGrpc<ServiceType>::Call *>(tag)->OnCompletionEvent(ok))
                    {
                        // static_cast<ServiceCallerGrpc<ServiceType>::Call*>(tag)->TryCancelCallAndShutdown();
                        RemoveCall(static_cast<ServiceCallerGrpc<ServiceType>::Call *>(tag)->GetId());
                    }
                }
            });
#endif
    }

    /*! 
     * Stop all pending rpc calls and close calls.
     * Finally, the message dispatcher therad will stop running.
     */
    void Stop()
    {
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

    /*!
     * Check if message dispatcher is running.
     * @return bool Whether message dispatcher thread is running.
     */
    bool IsRun() { return _running.load(); }

public:
    /*!
     * Call super class of all user-defined Call calsses.
     * This might be the parent class of all user defined Call implementations.
     */
    class Call
    {
    public:
        Call(typename ServiceType::Stub *stub, grpc::CompletionQueue *cq,
             void * /*placeholder for user data*/)
            : _stub(stub), _cq(cq)
        {
            _id = AllocCallId();
            _call_state = CallState::WAIT_CONNECT;
        }

        Call() = delete;
        virtual ~Call() = default;
        /*!
         * Completion event handler.
         * All subclasses should implement their own completion event handler properly.
         * This is called whenever async call requests completed such as Prepare(), Read(), Write(), Finalize(), etc.
         */
        virtual bool OnCompletionEvent(bool ok) = 0;

        /*!
         * Returns id for this Call instance.
         * @return call id.
         */
        uint64_t GetId() { return _id; }

        bool TryCancelCallAndShutdown()
        {
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

        enum class CallState
        {
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
        static uint64_t AllocCallId()
        {
            static std::atomic<uint64_t> _call_id_allocator{0};
            return (++_call_id_allocator);
        }
    };
    friend class Call;

public:
    /*!
     * Initiate a new RPC call.
     * The template parameter 'CallType' should be one of subclasses of dt::DAQ::ServiceCallerGrpc::Call.
     * User should subclass dt::DAQ::ServiceCallerGrpc::Call and implement their own completion event message handler.
     * @param[in] udata udata is passed as the sole argument of CallType constructor.
     */
    template <typename CallType> uint64_t StartCall(void *udata = nullptr)
    {
        std::shared_ptr<CallType> call =
            std::make_shared<CallType>(_stub.get(), &_cq, udata);
        std::lock_guard<std::mutex> lock(_call_list_mtx);
        _calls[call->GetId()] = call;
        return call->GetId();
    }

    /*!
     * Remove call by id.
     * It might be not called by user-code.
     * @param[in] call_id Id of Call instance to remove.
     * @return void
     */
    void RemoveCall(uint64_t call_id)
    {
        std::lock_guard<std::mutex> lock(_call_list_mtx);
        _calls.erase(call_id);
    }

    /*!
     * Get call by id.
     * @param[in] call_id Id of Call instance.
     * @return Shared pointer to the Call instance.
     */
    std::shared_ptr<Call> GetCall(uint64_t call_id)
    {
        auto call = _calls.find(call_id);
        if (call != _calls.end())
            return call->second;
        else
            return std::shared_ptr<Call>();
    }

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
/*! 
 * @example example_grpc_service_caller.cpp
 * This examples shows how to use ServiceCallerGrpc and ServiceCallerGrpc::Call 
 * for calling RPC at client side.
 * @see example_grpc_service_listener.cpp
 */

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_SERVICECALLERGRPC_H__