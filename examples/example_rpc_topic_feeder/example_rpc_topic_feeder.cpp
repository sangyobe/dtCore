#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcClient.h>
#include <dtCore/src/dtRpc/dtRpcTopicFeeder.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/mobile_ctrl_service.grpc.pb.h>


class SetMobileVelTopicFeeder : public dt::Rpc::TopicFeeder<rlab::ponytal::SetMobileVelReq, rlab::ponytal::SetMobileVelRsp>
{
    using MessageType = rlab::ponytal::SetMobileVelReq;
    using ResponseType = rlab::ponytal::SetMobileVelRsp;

public:
    SetMobileVelTopicFeeder(
        void *stub, 
        grpc::CompletionQueue *cq,
        void * udata)
        : dt::Rpc::TopicFeeder<MessageType , ResponseType>(stub, cq)
    {
        LOG(debug) << "SetMobileVelTopicFeeder::SetMobileVelTopicFeeder()";
        
        dt::Rpc::touchTimestamp(_message.mutable_header()->mutable_timestamp());
        _message.mutable_header()->set_seq(this->_id);

        // pre-allocate memory
        
        // prepare async RPC service
        _topic_writer = (static_cast<rlab::ponytal::MobileCtrlService::Stub*>(stub))->PrepareAsyncSetMobileVelStrm(&_client_ctx, &_response, cq);
        _session_state = SessionState::WAIT_CONNECT;
        _topic_writer->StartCall(&_call_context);
    }

    void OnResponse(const ResponseType &response) override
    {
        LOG(debug) << "SetMobileVelTopicFeeder::OnResponse()";
        LOG(trace) << "id:" << response.header().seq() << ", result:" << response.header().response_code();
    }
};

std::unique_ptr<dt::Rpc::Client<rlab::ponytal::MobileCtrlService>> client{nullptr};
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

        client = std::make_unique<dt::Rpc::Client<rlab::ponytal::MobileCtrlService> >("MobileCtrlService", address);
        // dt::Thread::SleepForMillis(1000);

        auto feeder = client->StartSession<SetMobileVelTopicFeeder>(nullptr);
        uint32_t msgSeq{0};

        while (bRun && client->IsRun())
        {
            rlab::ponytal::SetMobileVelReq msg;
            msg.mutable_header()->set_seq(msgSeq++);
            dt::Rpc::touchTimestamp(msg.mutable_header()->mutable_timestamp());
            msg.mutable_vel()->set_vx_mps(0.5f);
            msg.mutable_vel()->set_vy_mps(1.0f);
            msg.mutable_vel()->set_w_dps(45.0f);
            msg.set_time_sec(0.01f);
            feeder->Publish(msg);

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