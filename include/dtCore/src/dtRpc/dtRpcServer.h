// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_SERVER_H__
#define __DT_RPC_SERVER_H__

/** \defgroup dtRpc
 *
 */

#include <string>
#include <memory>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <grpcpp/grpcpp.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include "dtRpcServerSession.h"

namespace dt
{
namespace Rpc
{

class Server
{
public:
    //! Constructor
    Server(std::vector<std::pair<std::string, std::unique_ptr<::grpc::Service>>> &services, const std::string &server_address, const std::string &tls_server_cert = "", const std::string &tls_server_key = "")
    {
        grpc::EnableDefaultHealthCheckService(true);
        grpc::reflection::InitProtoReflectionServerBuilderPlugin();

        grpc::ServerBuilder builder;
        int selected_port;
        if (!tls_server_cert.empty() && !tls_server_key.empty())
        {
            // Load SSL
            grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair = {tls_server_key, tls_server_cert};
            grpc::SslServerCredentialsOptions ssl_options;
            ssl_options.pem_key_cert_pairs.emplace_back(key_cert_pair);
            builder.AddListeningPort(server_address, grpc::SslServerCredentials(ssl_options), &selected_port);
        }
        else
        {
            builder.AddListeningPort(server_address, grpc::InsecureServerCredentials(), &selected_port);
        }

        for (auto &service : services)
        {
            _services[service.first] = std::move(service.second);
            builder.RegisterService(_services[service.first].get());
        }

        /**
         * gRPC channel arguments
         *
         * GRPC_ARG_KEEPALIVE_TIME_MS
         *  - 이 시간(ms) 동안 트래픽이 없으면 PING 프레임 전송
         *  - 기본값: INT_MAX (사실상 비활성)
         *
         * GRPC_ARG_KEEPALIVE_TIMEOUT_MS
         *  - PING을 보낸 후 이 시간(ms) 내에 PONG이 오지 않으면 연결을 강제 종료
         *  - 타임아웃 시 채널 상태 → TRANSIENT_FAILURE → 재연결 시도
         *  - 기본값: 20000 ms (20초)
         *
         * GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS
         *  - 상대방(클라이언트)이 보내는 PING을 내가 허용하는 최소 간격
         *  - 이 간격보다 빠르게 PING이 오면 GOAWAY 프레임으로 연결 종료
         *  - 기본값: 300000 ms (5분)
         *
         * GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS
         *  - gRPC 내부 keepalive가 이 간격보다 빠르게 PING을 보내지 못하도록 하한선 설정
         *  - 실제 PING 전송 간격은 max(GRPC_ARG_KEEPALIVE_TIME_MS, GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS)
         *  - 기본값: 300000 ms (5분)
         *
         * GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS
         *  - 활성 RPC 호출이 없을 때도 PING을 보낼지 여부
         *  - 기본값: 0 (비활성 — RPC 있을 때만 PING)
         *
         * GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA
         *  - 데이터(또는 헤더) 없이 연속으로 보낼 수 있는 최대 PING 횟수
         *  - 기본값: 2 (2회 PING 전송 후 PING 전송 중단)
         *  - 0으로 설정할 때: PING 횟수 제한 해제(idle 상태에서도 keepalive가 끊기지 않고 계속 작동)
         *
         * GRPC_ARG_MAX_CONNECTION_IDLE_MS
         *  - 연결이 idle 상태로 유지될 수 있는 최대 시간(서버 전용)
         *  - 이 시간 동안 활성 RPC가 없으면 서버가 먼저 GOAWAY를 보내고 연결 종료
         *  - 기본값: INT_MAX (제한 없음)
         *
         * GRPC_ARG_MAX_CONNECTION_AGE_MS
         *  - 연결의 최대 수명(서버 전용)
         *  - 연결 생성 후 이 시간이 지나면 RPC 진행 중이어도 GOAWAY 전송 후 종료
         *  - 기본값: INT_MAX (제한 없음)
         *
         * GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS
         *  - 첫 번째 재연결 시도 전 대기 시간
         *  - 기본값: 1000 ms (1초)
         *
         * GRPC_ARG_MIN_RECONNECT_BACKOFF_MS / GRPC_ARG_MAX_RECONNECT_BACKOFF_MS
         *  - 재연결 backoff 간격의 하한/상한
         *  - 기본값: 1000 ms / 120000 ms (2분)
         *  - gRPC는 재연결 실패 시 exponential backoff 전략을 사용
         *      MIN(2000ms) / MAX(30000ms) 인 경우,
         *        1차 실패 → MIN 대기
         *        2차 실패 → 2000 × 1.6 = 3200ms 대기
         *        3차 실패 → 3200 × 1.6 = 5120ms 대기
         *        ...
         *        N차 실패 → MAX(300000ms) 상한에 도달하면 고정
         *  - MIN = MAX = 2000ms로 설정된 경우, backoff 없이 2초 고정 간격으로 재연결
         *  - 연결 끊김 → INITIAL_BACKOFF(3000ms) 1차 재연결 시도 → 실패 시 → MIN~MAX 범위로 backoff
         *
         * 클라이언트 KEEPALIVE 권장 설정
         *  GRPC_ARG_KEEPALIVE_TIME_MS,                             10000
         *  GRPC_ARG_KEEPALIVE_TIMEOUT_MS,                           5000
         *  GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS,                    1
         *  GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA,                      0
         *  GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS,  10000
         */
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 10000)
            .AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000)
            .AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1)
            .AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0) // idle 상태에서도 keepalive 완전 작동
            .AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS, INT_MAX)
            .AddChannelArgument(GRPC_ARG_MAX_CONNECTION_AGE_MS, INT_MAX)
            .AddChannelArgument(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 2000)
            .AddChannelArgument(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 2000)
            .AddChannelArgument(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 2000)
            .AddChannelArgument(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5000)
            .AddChannelArgument(GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, 5000);
        _cq = builder.AddCompletionQueue();
        _server = builder.BuildAndStart();
        if (!selected_port)
        {
            // LOG(err) << "Failed to bind listening port on " << server_address;
            throw std::runtime_error("Failed to bind listening port on " + server_address);
        }
        else
        {
            // LOG(info) << "Server listening on " << server_address;
        }

        // run cq-event loop
        Run();
    }
    //! Destructor
    virtual ~Server()
    {
        Stop();
    }

    //! Find a service
    grpc::Service *GetService(const std::string &service_name)
    {
        auto service = _services.find(service_name);
        if (service != _services.end())
            return service->second.get();
        else
            return nullptr;
    }

    //! Run grpc message-dispatcher
    void Run()
    {
        if (_running.load())
            return;

        _running.store(true);

        // rpc event "read done / write done / close(already connected)" call-back by the call completion queue 
        // rpc event "new connection / close(waiting for connect)" call-back by the notification completion queue
#if defined(_WIN32) || defined(__CYGWIN__)
        _rpc_event_thread = std::thread([this]
                                  {
            void *tag;
            bool ok;
            while (_cq->Next(&tag, &ok))
            {
                // GPR_ASSERT(ok);
                if (tag)
                {
                    Session *session = static_cast<CallContext *>(tag)->session;

                    if (!session->OnCompletionEvent(ok, tag))
                    {
                        session->TryCancelCallAndShutdown();
                        RemoveSession(session->GetId());
                    }
                }
            } });
#else
        pthread_create(
            &_rpc_event_thread, NULL,
            [](void *arg) -> void *
            {
                Server *server = (Server *)arg;

                void *tag;
                bool ok;
                while (server->_cq->Next(&tag, &ok))
                {
                    // GPR_ASSERT(ok);
                    if (tag)
                    {
                        Session *session = static_cast<CallContext *>(tag)->session;
                        
                        if (!session->OnCompletionEvent(ok, tag))
                        {
                            session->TryCancelCallAndShutdown();
                            server->RemoveSession(session->GetId());
                        }
                    }
                }
                return 0;
            },
            (void *)this);
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

        _server->Shutdown(); // TODO: 위 세션 셧다운 전에 해야하는 거 아닌가?
        _cq->Shutdown(); // Always *after* the associated server's Shutdown()!
        // drain the queue
        void *ignored_tag = nullptr;
        bool ignored_ok = false;
        while (_cq->Next(&ignored_tag, &ignored_ok)) {}

        _running.store(false);
#if defined(_WIN32) || defined(__CYGWIN__)
        if (_rpc_event_thread.joinable())
        {
            _rpc_event_thread.join();
        }
#else
        if (_rpc_event_thread)
        {
            pthread_join(_rpc_event_thread, nullptr);
            _rpc_event_thread = (pthread_t) nullptr;
        }
#endif
    }

    //! Check if the server is running
    bool IsRun() { return _running.load(); }

    //! Set server serving(health) status
    void SetServingStatus(const std::string& service_name, bool serving)
    {
        grpc::HealthCheckServiceInterface* health_service = _server->GetHealthCheckService();
        if (health_service != nullptr) 
        {
            health_service->SetServingStatus(service_name, serving);
        }
    }

    /*!
     * Initiate a new RPC session.
     * The template parameter 'SessionType' should be one of subclasses of dt::Rpc::ServerSession.
     * User should subclass dt::Rpc::ServerSession and implement their own completion event message handler.
     * @param[in] udata udata is passed as the sole argument of SessionType constructor.
     */
    template <typename SessionType> std::shared_ptr<SessionType> AddSession(const std::string &service_name, void *udata = nullptr)
    {
        ::grpc::Service *service = GetService(service_name);
        if (!service)
            return nullptr;

        std::shared_ptr<SessionType> session = std::make_shared<SessionType>(this, service, _cq.get(), _cq.get(), udata);
        std::lock_guard<std::mutex> lock(_session_list_mtx);
        _sessions[session->GetId()] = session;
        return session;
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

    /*!
     * Update sessions.
     */
    void UpdateSessions()
    {
        std::lock_guard<std::mutex> lock(_session_list_mtx);
        for (auto it : _sessions)
        {
            it.second->StreamData();
        }
    }

    /*!
     * Publish(Broadcast) a message.
     * This function is for broadcasting a message to all sessions which are sub-classes of 'SessionType'.
     * @param[in] message A message to publish.
     */
    template<typename SessionType, typename MessageType>
    void Publish(const MessageType &message)
    {
        std::lock_guard<std::mutex> lock(_session_list_mtx);
        for (auto it : _sessions)
        {
            std::shared_ptr<SessionType> session = std::dynamic_pointer_cast<SessionType>(it.second);
            if (session) // TODO: make it sure that session is a subclass instance of TopicPublisher!
                session->Publish(message);
        }
    }

private:
    std::unique_ptr<grpc::Server> _server;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    // std::unique_ptr<grpc::Service> _service;
    std::unordered_map<std::string, std::unique_ptr<::grpc::Service>> _services;
    std::string _server_address;
    std::atomic<bool> _running{false};
#if defined(_WIN32) || defined(__CYGWIN__)
    std::thread _rpc_event_thread;
#else
    pthread_t _rpc_event_thread{0};
#endif
    std::mutex _session_list_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<ServerSession>> _sessions;

private: // deleted default copy&assignment constructors
    Server(const Server &) = delete;
    Server(Server &&) = delete;
    Server &operator=(const Server &) = delete;
    Server &operator=(Server &&) = delete;
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_SERVER_H__
