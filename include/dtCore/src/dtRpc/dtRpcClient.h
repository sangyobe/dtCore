// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_CLIENT_H__
#define __DT_RPC_CLIENT_H__

/** \defgroup dtRpc
 *
 */

#include "dtRpcClientSession.h"
#include <dtCore/dtLog>
#include <functional>
#include <atomic>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>

namespace dt
{
namespace Rpc
{

/*!
 * Connection state.
 */
typedef enum
{
    /** channel is idle */
    CONN_IDLE = GRPC_CHANNEL_IDLE,
    /** channel is connecting */
    CONN_CONNECTING = GRPC_CHANNEL_CONNECTING,
    /** channel is ready for work */
    CONN_READY = GRPC_CHANNEL_READY,
    /** channel has seen a failure but expects to recover */
    CONN_TRANSIENT_FAILURE = GRPC_CHANNEL_TRANSIENT_FAILURE,
    /** channel has seen a failure that it cannot recover from */
    CONN_SHUTDOWN = GRPC_CHANNEL_SHUTDOWN,
    /** unknown */
    CONN_UNKNOWN = -1
} CONN_STATE;

class ConnContext
{
public:
    ConnContext(grpc_connectivity_state conn_state_) : conn_state(conn_state_) {}
    grpc_connectivity_state conn_state;

private:
    ConnContext() = delete;
};

template <typename ServiceType>
class Client
{
public:
    Client(const std::string &name, const std::string &server_address, const std::string &tls_root_cert = "", std::function<void(CONN_STATE)> conn_handler = nullptr, bool auto_reconnect = false)
        : _name(name), _conn_handler(conn_handler), _auto_reconnect(auto_reconnect)
    {
        grpc::ChannelArguments ch_args;
        ch_args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 10000);
        ch_args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 5000);
        ch_args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        ch_args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        ch_args.SetInt(GRPC_ARG_MAX_CONNECTION_IDLE_MS, INT_MAX);
        ch_args.SetInt(GRPC_ARG_MAX_CONNECTION_AGE_MS, INT_MAX);
        ch_args.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 2000);
        ch_args.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 1000);
        ch_args.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 1000);
        ch_args.SetInt(GRPC_ARG_HTTP2_MIN_RECV_PING_INTERVAL_WITHOUT_DATA_MS, 5000);
        ch_args.SetInt(GRPC_ARG_HTTP2_MIN_SENT_PING_INTERVAL_WITHOUT_DATA_MS, 5000);

        std::shared_ptr<grpc::ChannelCredentials> ch_creds;
        if (!tls_root_cert.empty())
        {
            grpc::SslCredentialsOptions ssl_options;
            ssl_options.pem_root_certs = tls_root_cert;
            ch_creds = grpc::SslCredentials(ssl_options);
        }
        else
        {
            ch_creds = grpc::InsecureChannelCredentials();
        }

        // connect(make a channel) to server and create a stub
        _channel = grpc::CreateCustomChannel(server_address, ch_creds, ch_args);
        _stub = ServiceType::NewStub(_channel);

        // run cq-event loop
        Run();
    }

    virtual ~Client()
    {
        Stop();
    }

public:
    /*!
     * Run grpc message-dispatcher.
     * It launches a thread to check grpc I/O completion.
     * User does not need to call Run() explicitly. dtServiceCallGrpc() constructor will do.
     */
    void Run()
    {
        if (_running.load(std::memory_order_acquire))
            return;

        _running.store(true, std::memory_order_release);

        // create a completion queue for monitoring channel state
        if (!_cq_reconn)
        {
            _cq_reconn = std::make_unique<grpc::CompletionQueue>();
            ConnContext *conn_ctx = new ConnContext(_channel->GetState(false));                 // _channel->GetState(false)); // GRPC_CHANNEL_IDLE);
            auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(1000); // std::chrono::system_clock::time_point::max()
            _channel->NotifyOnStateChange(conn_ctx->conn_state, deadline, _cq_reconn.get(), (void *)(conn_ctx));
            LOG(debug) << _name << ": " << "NotifyOnStateChange req: conn_ctx[" << conn_ctx << "], channel state: " << conn_ctx->conn_state;
        }

        // start channel(connection) monitoring thread
#if defined(_WIN32) || defined(__CYGWIN__)
        _conn_event_thread = std::thread([this]
                                         {
            void *tag;
            bool ok;
            while (_cq_reconn->Next(&tag, &ok))
            {
                LOG(trace) << "_cq_reconn->Next() ok: " << ok << ", tag: " << tag;

                if (ok && tag)
                {
                    ConnContext *conn_ctx = static_cast<ConnContext *>(tag);
                    LOG(debug) << _name << ": " << "on conn_ctx[" << tag << "] Channel state: " << conn_ctx->conn_state;
                    OnConnectionEvent(*conn_ctx);
                    delete conn_ctx;
                }
                else
                {
                    if (tag) delete static_cast<ConnContext *>(tag);

                    if (_running.load(std::memory_order_acquire))
                    {
                        ConnContext *conn_ctx = new ConnContext(_channel->GetState(false));
                        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);
                        _channel->NotifyOnStateChange(conn_ctx->conn_state, deadline, _cq_reconn.get(), (void *)(conn_ctx));
                        LOG(trace) << _name << ": " << "NotifyOnStateChange req: conn_ctx[" << conn_ctx << "], channel state: " << conn_ctx->conn_state;
                    }
                    else
                    {
                        break;
                    }
                }
            } });
#else
        pthread_create(
            &_conn_event_thread, NULL,
            [](void *arg) -> void *
            {
                Client<ServiceType> *client = (Client<ServiceType> *)arg;
                void *tag;
                bool ok;
                while (client->_cq_reconn->Next(&tag, &ok))
                {
                    LOG(trace) << "_cq_reconn->Next() ok: " << ok << ", tag: " << tag;

                    if (ok && tag)
                    {
                        ConnContext *conn_ctx = static_cast<ConnContext *>(tag);
                        LOG(debug) << client->_name << ": " << "on conn_ctx[" << tag << "] Channel state: " << conn_ctx->conn_state;
                        client->OnConnectionEvent(*conn_ctx);
                        delete conn_ctx;
                    }
                    else
                    {
                        if (tag) delete static_cast<ConnContext *>(tag);

                        if (client->_running.load(std::memory_order_acquire))
                        {
                            ConnContext *conn_ctx = new ConnContext(client->_channel->GetState(false));
                            auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);
                            client->_channel->NotifyOnStateChange(conn_ctx->conn_state, deadline, client->_cq_reconn.get(), (void *)(conn_ctx));
                            LOG(trace) << client->_name << ": " << "NotifyOnStateChange req: conn_ctx[" << conn_ctx << "], channel state: " << client->_channel->GetState(false);

                        }
                        else
                        {
                            break;
                        }
                    }
                }
                return 0;
            },
            (void *)this);
#endif

        // create a completion queue for being signaled of I/O completion
        if (!_cq)
        {
            _cq = std::make_unique<grpc::CompletionQueue>();
        }

        // rpc event "read done / write done / close(already connected)" call-back
        // by this call completion queue rpc event "new connection / close(waiting
        // for connect)" call-back by this notification completion queue
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
                        // session->TryCancelCallAndShutdown();
                        RemoveSession(session->GetId());
                    }
                }
            }
            this->_running.store(false, std::memory_order_release); });
#else
        pthread_create(
            &_rpc_event_thread, NULL,
            [](void *arg) -> void *
            {
                Client<ServiceType> *client =
                    (Client<ServiceType> *)arg;

                void *tag;
                bool ok;
                while (client->_cq->Next(&tag, &ok))
                {
                    // GPR_ASSERT(ok);
                    if (tag)
                    {
                        Session *session = static_cast<CallContext *>(tag)->session;

                        if (!session->OnCompletionEvent(ok, tag))
                        {
                            // session->TryCancelCallAndShutdown();
                            client->RemoveSession(session->GetId());
                        }
                    }
                }
                client->_running.store(false, std::memory_order_release);
                return 0;
            },
            (void *)this);
#endif
    }

    /*!
     * Stop all pending rpc calls and close calls.
     * Finally, the message dispatcher therad will stop running.
     */
    void Stop()
    {
        _running.store(false, std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(_session_list_mtx);
            for (auto it : _sessions)
            {
                it.second->TryCancelCallAndShutdown();
            }
            // _sessions.clear();
        }

        //
        // shutdown rpc I/O completion queue
        _cq->Shutdown();

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

        // drain the queue
        void *ignoredTag = nullptr;
        bool ok = false;
        while (_cq->Next(&ignoredTag, &ok)) {}

        // shutdown connection monitoring completion queue
        _cq_reconn->Shutdown();

#if defined(_WIN32) || defined(__CYGWIN__)
        if (_conn_event_thread.joinable())
        {
            _conn_event_thread.join();
        }
#else
        if (_conn_event_thread)
        {
            pthread_join(_conn_event_thread, nullptr);
            _conn_event_thread = (pthread_t) nullptr;
        }
#endif

        // drain the queue
        // void *ignoredTag = nullptr;
        // bool ok = false;
        while (_cq_reconn->Next(&ignoredTag, &ok))
        {
            if (ignoredTag) delete static_cast<ConnContext *>(ignoredTag);
        }

    }

    /*!
     * Check if message dispatcher is running.
     * @return bool true if the message dispatcher thread is running.
     */
    bool IsRun() { return _running.load(std::memory_order_acquire); }

    /*!
     * Get gprc channel(connection) state.
     * @return CONN_STATE the current grpc connection state.
     */
    CONN_STATE GetConnectionState()
    {
        if (!_channel)
            return CONN_UNKNOWN;
        else
            return (CONN_STATE)_channel->GetState(false);
    }

    /*!
     * Check if a connection(channel) is establised to a grpc server.
     * @return bool true if the connection is established.
     */
    bool IsConnected()
    {
        if (!_channel)
            return false;

        grpc_connectivity_state state = _channel->GetState(false);
        return (state == grpc_connectivity_state::GRPC_CHANNEL_READY);
    }

    /*!
     * Try to reconnect to the grpc server.
     */
    void Reconnect()
    {
        if (!_channel)
            return;

        (void)_channel->GetState(true);
    }

    void OnConnectionEvent(ConnContext &conn_ctx_from)
    {
        ConnContext *conn_ctx = new ConnContext(_channel->GetState(false));
        LOG(debug) << _name << ": " << "Channel state changed: " << conn_ctx_from.conn_state << " -> " << conn_ctx->conn_state;
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(1000);
        _channel->NotifyOnStateChange(conn_ctx->conn_state, deadline, _cq_reconn.get(), (void *)(conn_ctx));
        LOG(trace) << _name << ": " << "NotifyOnStateChange req: conn_ctx[" << conn_ctx << "], channel state: " << conn_ctx->conn_state;
        if (_conn_handler)
            _conn_handler((CONN_STATE)(conn_ctx->conn_state));
        if (_auto_reconnect && (conn_ctx->conn_state == GRPC_CHANNEL_IDLE || conn_ctx->conn_state == GRPC_CHANNEL_TRANSIENT_FAILURE))
            Reconnect();
    }

    /*!
     * Initiate a new RPC call.
     * The template parameter 'SessionType' should be one of subclasses of dt::Rpc::ClientSession.
     * User should subclass dt::Rpc::ClientSession and implement their own completion event message handler.
     * @param[in] udata udata is passed as the sole argument of SessionType constructor.
     */
    template <typename SessionType> std::shared_ptr<SessionType> StartSession(void *udata = nullptr)
    {
        std::shared_ptr<SessionType> session =
            std::make_shared<SessionType>(_stub.get(), _cq.get(), udata);
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

private:
    std::string _name{""};
    std::shared_ptr<grpc::Channel> _channel{nullptr};
    std::unique_ptr<typename ServiceType::Stub> _stub{nullptr};
    std::unique_ptr<grpc::CompletionQueue> _cq{nullptr};
    std::unique_ptr<grpc::CompletionQueue> _cq_reconn{nullptr};
    std::function<void(CONN_STATE)> _conn_handler{nullptr};
    std::atomic<bool> _auto_reconnect{false};
    std::atomic<bool> _running{false};
#if defined(_WIN32) || defined(__CYGWIN__)
    std::thread _conn_event_thread;
    std::thread _rpc_event_thread;
#else
    pthread_t _conn_event_thread{0};
    pthread_t _rpc_event_thread{0};
#endif
    std::mutex _session_list_mtx;
    std::unordered_map<uint64_t, std::shared_ptr<Session>> _sessions;

private: // deleted default copy&assignment constructors
    Client<ServiceType>(const Client<ServiceType> &) = delete;
    Client<ServiceType>(Client<ServiceType> &&) = delete;
    Client<ServiceType> &operator=(const Client<ServiceType> &) = delete;
    Client<ServiceType> &operator=(Client<ServiceType> &&) = delete;

    friend class Session;
    friend class ClientSession;
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_CLIENT_H__