#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcClient.h>
#include <dtCore/src/dtRpc/dtRpcTopicSubscriber.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/robot_state_service.grpc.pb.h>

class RobotStateTopicSubscriber : public dt::Rpc::TopicSubscriber<rlab::ponytal::GetRobotStateReq, rlab::ponytal::GetRobotStateRsp>
{
    using RequestType = rlab::ponytal::GetRobotStateReq;
    using MessageType = rlab::ponytal::GetRobotStateRsp;

public:
    RobotStateTopicSubscriber(
        void *stub,
        grpc::CompletionQueue *cq,
        void *udata)
        : TopicSubscriber<RequestType, MessageType>(stub, cq)
    {
        LOG(debug) << "RobotStateTopicSubscriber::RobotStateTopicSubscriber()";
        dt::Rpc::touchTimestamp(_request.mutable_header()->mutable_timestamp());
        _request.mutable_header()->set_seq(this->_id);
        _topic_reader = (static_cast<rlab::ponytal::RobotStateService::Stub *>(stub))->PrepareAsyncGetRobotStateStrm(&_client_ctx, _request, cq);
        _session_state = SessionState::WAIT_CONNECT;
        _topic_reader->StartCall(&_call_context);
    }

    void OnMessage(const MessageType &message) override
    {
        LOG(debug) << "RobotStateTopicSubscriber::OnMessage()";
        LOG(trace) << "hdr.id:" << message.header().seq() << ", msg:" << message.header().response_desc();
        int jid = 0;
        for (auto state : message.state().jnt_states())
        {
            LOG(trace) << "jnt_states[" << jid << "].pos:" << state.pos();
            LOG(trace) << "jnt_states[" << jid << "].vel:" << state.vel();
            LOG(trace) << "jnt_states[" << jid << "].tau:" << state.tau();
            jid++;
        }
        LOG(trace) << "mobile_state.moving:" << message.state().mobile_state().moving();
        LOG(trace) << "mobile_state.vel.vx_mps:" << message.state().mobile_state().vel().vx_mps();
        LOG(trace) << "mobile_state.vel.vy_mps:" << message.state().mobile_state().vel().vy_mps();
        LOG(trace) << "mobile_state.vel.w_dps:" << message.state().mobile_state().vel().w_dps();
    }
};

std::unique_ptr<dt::Rpc::Client<rlab::ponytal::RobotStateService>> client{nullptr};
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
        std::string address = "localhost:50031";

        client = std::make_unique<dt::Rpc::Client<rlab::ponytal::RobotStateService> >("RobotStateService", address);
        // dt::Thread::SleepForMillis(1000);

        client->StartSession<RobotStateTopicSubscriber>(nullptr);

        while (bRun && client->IsRun())
        {

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        LOG(info) << "> stopping...";
        client->Stop();
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