#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcClient.h>
#include <dtCore/src/dtRpc/dtRpcActionClient.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/behavior_ctrl_service.grpc.pb.h>

class SetBehaviorActionClient : public dt::Rpc::ActionClient<rlab::ponytal::SetBehaviorReq, rlab::ponytal::SetBehaviorRsp>
{
    using RequestType = rlab::ponytal::SetBehaviorReq;
    using ResponseType = rlab::ponytal::SetBehaviorRsp;
public:
    SetBehaviorActionClient(
        void *stub, 
        grpc::CompletionQueue *cq,
        void * udata)
        : dt::Rpc::ActionClient<RequestType , ResponseType>(stub, cq)
    {
        LOG(debug) << "SetBehaviorActionClient::SetBehaviorActionClient()";
        
        dt::Rpc::touchTimestamp(_request.mutable_header()->mutable_timestamp());
        _request.mutable_header()->set_seq(this->_id);

        // set behavior
        _request.set_behavior("btree.xml");

        // prepare async RPC service
        _response_reader = (static_cast<rlab::ponytal::BehaviorCtrlService::Stub*>(stub))->PrepareAsyncSetBehavior(&_client_ctx, _request, cq);
        _session_state = SessionState::WAIT_CONNECT;
        _response_reader->StartCall(&_call_context);
    }

    void OnFeedback(const ResponseType &feedback) override
    {
        LOG(debug) << "SetBehaviorActionClient::OnFeedback()";
        LOG(trace) << "id:" << feedback.header().seq() << ", result:" << feedback.header().response_code();
    }

    void OnResponse(const ResponseType &response) override
    {
        LOG(debug) << "SetBehaviorActionClient::OnResponse()";
        LOG(trace) << "id:" << response.header().seq() << ", result:" << response.header().response_code();
    }
};

std::unique_ptr<dt::Rpc::Client<rlab::ponytal::BehaviorCtrlService>> client{nullptr};
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

        client = std::make_unique<dt::Rpc::Client<rlab::ponytal::BehaviorCtrlService> >("BehaviorCtrlService", address);
        // dt::Thread::SleepForMillis(1000);

        // client->StartSession<SetBehaviorActionClient>(nullptr);

        while (bRun && client->IsRun())
        {
            std::cout << "\n\'test\' : set test behavior";
            std::cout << "\n\n(type \'q\' to quit) >\n";
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "q" || cmd == "quit") { bRun = false; }
            else if (cmd == "test")  { client->StartSession<SetBehaviorActionClient>(nullptr); }

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