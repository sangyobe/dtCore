#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcServer.h>
#include <dtCore/src/dtRpc/dtRpcTopicListener.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/mobile_ctrl_service.grpc.pb.h>

class SetMobileVelTopicListener : public dt::Rpc::TopicListener<rlab::ponytal::SetMobileVelReq, rlab::ponytal::SetMobileVelRsp>
{
    using MessageType = rlab::ponytal::SetMobileVelReq;
    using ResponseType = rlab::ponytal::SetMobileVelRsp;

public:
    SetMobileVelTopicListener(
        dt::Rpc::Server *server,
        grpc::Service *service,
        grpc::CompletionQueue *cq_call,
        grpc::ServerCompletionQueue *cq_ntfy,
        void *udata)
        : TopicListener<MessageType, ResponseType>(server, service, cq_call, cq_ntfy), _udata(udata)
    {
        // prepare async RPC service
        _session_state = SessionState::WAIT_CONNECT;
        LOG(debug) << "NEW SetMobileVelTopicListener(" << this->_id << ") created.";
        (static_cast<rlab::ponytal::MobileCtrlService::AsyncService *>(service))->RequestSetMobileVelStrm(&_server_ctx, &_topic_reader, cq_call, cq_ntfy, &_call_context);
    }

    ~SetMobileVelTopicListener()
    {
        LOG(debug) << "SetMobileVelTopicListener(" << this->_id << ") destroyed.";
    }

    void OnConnect() override
    {
        LOG(info) << "SetMobileVelTopicListener connected: " << dt::Rpc::getReadablePeerAddress(_server_ctx.peer());
        _server->AddSession<SetMobileVelTopicListener>(rlab::ponytal::MobileCtrlService::service_full_name(), _udata);
    }

    void OnMessage(const MessageType &message) override
    {
        LOG(trace) << "SetMobileVelTopicListener::OnMessage()";
        LOG(trace) << "hdr.seq:" << message.header().seq();

        // LOG(trace) << "tgt.vx_mps :" << message.vel().vx_mps();
        // LOG(trace) << "tgt.vy_mps :" << message.vel().vy_mps();
        // LOG(trace) << "tgt.w_dps :" << message.vel().w_dps();
        // LOG(trace) << "tgt.time_sec :" << message.time_sec();
        LOG(trace).format("cmd_vel: {:.3f} {:.3f} {:.3f}", message.vel().vx_mps(), message.vel().vy_mps(), message.vel().w_dps());
    }

private:
    void *_udata{nullptr};
};

std::unique_ptr<dt::Rpc::Server> server{nullptr};
std::atomic<bool> bRun{true};

void CatchSignal(int sig)
{
    LOG(info) << "catch signal: " << sig;
    bRun = false;
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, CatchSignal); // kill command
    signal(SIGINT, CatchSignal);  // keyboard interrupt, Ctrl + c

    dt::Log::Initialize(argv[0]);
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    LOG(info) << "dtRpc test client.";

    try
    {
        std::string address = "0.0.0.0:50031";

        std::string serviceName = rlab::ponytal::MobileCtrlService::service_full_name();
        std::vector<std::pair<std::string, std::unique_ptr<::grpc::Service>>> services;
        services.emplace_back(serviceName, std::make_unique<rlab::ponytal::MobileCtrlService::AsyncService>());
        server = std::make_unique<dt::Rpc::Server>(services, address);
        server->AddSession<SetMobileVelTopicListener>(serviceName, nullptr);
        // dt::Thread::SleepForMillis(1000);

        while (bRun)
        {
            server->UpdateSessions();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG(info) << "> stopping...";
        server->Stop();
        server.reset();
        LOG(info) << "> stopped.";
        LOG(info) << "> Press any key and [Enter] to exit...";
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    LOG(info) << "Program terminated.";
    dt::Log::Terminate();
    
    return 0;
}