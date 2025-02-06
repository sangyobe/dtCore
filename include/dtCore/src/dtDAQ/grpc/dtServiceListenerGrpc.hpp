// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_DAQ_SERVICELISTENERGRPC_H__
#define __DT_DAQ_SERVICELISTENERGRPC_H__

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
#include <deque>
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
 * @brief ServiceListenerGrpc class.
 * @details Is serves as the main interface for RPC server.
 * It owns and runs RPC event message dispatcher thread.
 * To initiate a new RPC server, use AddSession() method with proper subclass of ServiceListenerGrpc::Session template parameter.
 */
class ServiceListenerGrpc
{
public:
    ServiceListenerGrpc(std::unique_ptr<grpc::Service> service,
                        const std::string &server_address)
        : _service(std::move(service)), _server_address(server_address)
    {
        grpc::ServerBuilder builder;
        builder.AddListeningPort(_server_address,
                                 grpc::InsecureServerCredentials());
        builder.RegisterService(_service.get());
        _cq = builder.AddCompletionQueue();
        _server = builder.BuildAndStart();
        Run();
    }

    ~ServiceListenerGrpc() { Stop(); }

public:
    //! Run grpc message-dispatcher
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
                ServiceListenerGrpc *client = (ServiceListenerGrpc *)arg;

                void *tag;
                bool ok;
                while (client->_cq->Next(&tag, &ok))
                {
                    // GPR_ASSERT(ok);
                    if (tag)
                    {
                        if (!static_cast<ServiceListenerGrpc::Session *>(tag)->OnCompletionEvent(ok))
                        {
                            static_cast<ServiceListenerGrpc::Session *>(tag)->TryCancelCallAndShutdown();
                            client->RemoveSession(static_cast<ServiceListenerGrpc::Session *>(tag)->GetId());
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
            while (_cq->Next(&tag, &ok))
            {
                // GPR_ASSERT(ok);
                if (tag)
                {
                    if (!static_cast<ServiceListenerGrpc::Session *>(tag)->OnCompletionEvent(ok))
                    {
                        static_cast<ServiceListenerGrpc::Session *>(tag)->TryCancelCallAndShutdown();
                        RemoveSession(static_cast<ServiceListenerGrpc::Session *>(tag)->GetId());
                    }
                }
            }
        });
#endif
    }

    //! Stop all pending rpc calls and close sessions
    void Stop()
    {
        {
            std::lock_guard<std::mutex> lock(_session_list_mtx);
            for (auto it : _sessions)
            {
                it.second->TryCancelCallAndShutdown();
            }
            // _sessions.clear();
        }

        _running = false;
        _server->Shutdown();
        _cq->Shutdown(); // drain/signal all completion queue.

#ifdef USE_THREAD_PTHREAD
        if (_rpc_thread)
        {
            pthread_join(_rpc_thread, nullptr);
            _rpc_thread = (pthread_t)nullptr;
        }
#else
        if (_rpc_thread.joinable())
        {
            _rpc_thread.join();
        }
#endif

        // drain the queue
        void *ignoredTag = nullptr;
        bool ok = false;
        while (_cq->Next(&ignoredTag, &ok)) {}
    }
    bool IsRun() { return _running.load(); }

public:
    class Session
    {
    public:
        Session(ServiceListenerGrpc *server, grpc::Service *service,
                grpc::ServerCompletionQueue *cq,
                void * /*placeholder for user data*/)
            : _server(server), _service(service), _cq(cq),
              _call_state(CallState::WAIT_CONNECT)
        {
            _id = AllocSessionId();
            _call_state = CallState::WAIT_CONNECT;
        }

        Session() = delete;
        virtual ~Session() = default;

        /*!
         * Completion event handler.
         * All subclasses should implement their own completion event handler properly.
         * This is called whenever async call requests completed such as Prepare(), Read(), Write(), Finalize(), etc.
         * @return true if message is handled successfully. if it returns false, this Call object will be deleted.
         */
        virtual bool OnCompletionEvent(bool ok) = 0;

        /*!
         * Send message interface.
         * This may be implemented by user's sub-class implementation of Session.
         * @param[in] udata placeholder for user data.
         * @return true if it sends a message successfully.
         */
        virtual bool Send(void *udata = nullptr) { return false; }

        /*!
         * Returns id for this Session instance.
         * @return session id.
         */
        uint64_t GetId() { return _id; }

        void TryCancelCallAndShutdown()
        {
            // std::lock_guard<std::mutex> lock(_proc_mtx);
            if (_call_state != CallState::WAIT_CONNECT &&
                _call_state != CallState::WAIT_FINISH &&
                _call_state != CallState::FINISHED)
            {
                _ctx.TryCancel();

                std::lock_guard<std::mutex> lock(_proc_mtx);
                //_call_state = CallState::WAIT_FINISH;
                _call_state = CallState::FINISHED;
            }

            // _call_state = CallState::FINISHED;
            // _server->RemoveSession(_id);
        }

    protected:
        uint64_t _id;
        ServiceListenerGrpc *_server;
        grpc::Service *_service;
        grpc::ServerCompletionQueue *_cq;
        grpc::ServerContext _ctx;
        std::mutex _proc_mtx;

        enum class CallState
        {
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
        static uint64_t AllocSessionId()
        {
            static std::atomic<uint64_t> _session_id_allocator{0};
            return (++_session_id_allocator);
        }
    };
    friend class Session;

    /*!
     * Initiate a new RPC session.
     * The template parameter 'SessionType' should be one of subclasses of dt::DAQ::ServiceListenerGrpc::Session.
     * User should subclass dt::DAQ::ServiceListenerGrpc::Session and implement their own completion event message handler.
     * @param[in] udata udata is passed as the sole argument of SessionType constructor.
     */
    template <typename SessionType> uint64_t AddSession(void *udata = nullptr)
    {
        std::shared_ptr<SessionType> session =
            std::make_shared<SessionType>(this, _service.get(), _cq.get(), udata);
        std::lock_guard<std::mutex> lock(_session_list_mtx);
        _sessions[session->GetId()] = session;
        return session->GetId();
    }

    /*!
     * Remove session by id.
     * It might be not called by user-code.
     * @param[in] session_id Id of Session instance to remove.
     */
    void RemoveSession(uint64_t session_id)
    {
        std::lock_guard<std::mutex> lock(_session_list_mtx);
        _sessions.erase(session_id);
    }

    /*!
     * Get session by id.
     * @param[in] session_id Id of Session instance.
     * @return Shared pointer to the Session instance.
     */
    std::shared_ptr<Session> GetSession(uint64_t session_id)
    {
        auto session = _sessions.find(session_id);
        if (session != _sessions.end())
            return session->second;
        else
            return std::shared_ptr<Session>();
    }

protected:
    std::unique_ptr<grpc::Server> _server;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    std::unique_ptr<grpc::Service> _service;
    std::string _server_address;
    std::atomic<bool> _running{false};
#ifdef USE_THREAD_PTHREAD
    pthread_t _rpc_thread;
#else
    std::thread _rpc_thread;
#endif
    std::mutex _session_list_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<Session>> _sessions;
};
/*! 
 * @example example_grpc_service_listener.cpp
 * This examples shows how to use ServiceListenerGrpc and ServiceListenerGrpc::Session 
 * for handling remove RPC call at server side.
 * @see example_grpc_service_caller.cpp
 */

} // namespace DAQ
} // namespace dt

#endif // __DT_DAQ_SERVICELISTENERGRPC_H__