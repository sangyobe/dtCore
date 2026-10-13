#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcClient.h>
#include <dtCore/src/dtRpc/dtRpcServiceClient.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/sys_ctrl_service.grpc.pb.h>

class GetPermissionServiceClient : public dt::Rpc::ServiceClient<rlab::ponytal::GetPermissionReq, rlab::ponytal::GetPermissionRsp>
{
    using RequestType = rlab::ponytal::GetPermissionReq;
    using ResponseType = rlab::ponytal::GetPermissionRsp;

public:
    GetPermissionServiceClient(
        void *stub, 
        grpc::CompletionQueue *cq,
        void * udata)
        : dt::Rpc::ServiceClient<RequestType , ResponseType>(stub, cq)
    {
        LOG(debug) << "GetPermissionServiceClient::GetPermissionServiceClient()";
        
        dt::Rpc::touchTimestamp(_request.mutable_header()->mutable_timestamp());
        _request.mutable_header()->set_seq(this->_id);
        _request.set_permission(rlab::ponytal::PERMISSION_SET_CTRL_MODE);
        _request.set_arm_ctrl_mode(rlab::ponytal::ARM_CTRL_MODE_SET_GOAL);
        _request.set_mobile_ctrl_mode(rlab::ponytal::MOBILE_CTRL_MODE_NAVI_STRM);

        _response_reader = (static_cast<rlab::ponytal::SysCtrlService::Stub*>(stub))->PrepareAsyncGetPermission(&_client_ctx, _request, cq);
        _session_state = SessionState::WAIT_FINISH;
        _response_reader->StartCall();
        _response_reader->Finish(&_response, &_call_status, &_call_context);
    }

    void OnResponse(const ResponseType &response) override
    {
        LOG(debug) << "GetPermissionServiceClient::OnResponse()";
        LOG(trace) << "id:" << response.header().seq() << ", error:" << response.error();
    }
};

std::unique_ptr<dt::Rpc::Client<rlab::ponytal::SysCtrlService>> client{nullptr};
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

        client = std::make_unique<dt::Rpc::Client<rlab::ponytal::SysCtrlService> >("SysCtrlService", address);
        // dt::Thread::SleepForMillis(1000);

        while (bRun && client->IsRun())
        {
            std::cout << "\n\'test\' : set test permission";
            std::cout << "\n\n(type \'q\' to quit) >\n";
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "q" || cmd == "quit") { bRun = false; }
            else if (cmd == "test")  { client->StartSession<GetPermissionServiceClient>(nullptr); }

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