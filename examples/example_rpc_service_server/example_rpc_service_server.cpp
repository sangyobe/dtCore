#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcServer.h>
#include <dtCore/src/dtRpc/dtRpcServiceServer.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/sys_ctrl_service.grpc.pb.h>

class GetPermissionServiceServer : public dt::Rpc::ServiceServer<rlab::ponytal::GetPermissionReq, rlab::ponytal::GetPermissionRsp>
{
    using RequestType = rlab::ponytal::GetPermissionReq;
    using ResponseType = rlab::ponytal::GetPermissionRsp;

public:
    GetPermissionServiceServer(
        dt::Rpc::Server *server,
        grpc::Service *service,
        grpc::CompletionQueue *cq_call,
        grpc::ServerCompletionQueue *cq_ntfy,
        void *udata)
        : ServiceServer<RequestType, ResponseType>(server, service, cq_call, cq_ntfy), _udata(udata)
    {
        // prepare async RPC service
        _session_state = SessionState::WAIT_CONNECT;
        LOG(debug) << "NEW GetPermissionServiceServer(" << this->_id << ") created.";
        (static_cast<rlab::ponytal::SysCtrlService::AsyncService *>(service))->RequestGetPermission(&_server_ctx, &_request, &_responder, cq_call, cq_ntfy, &_call_context);
    }

    ~GetPermissionServiceServer()
    {
        LOG(debug) << "GetPermissionServiceServer(" << this->_id << ") destroyed.";
    }

    void OnConnect() override
    {
        LOG(info) << "GetPermissionServiceServer connected: " << dt::Rpc::getReadablePeerAddress(_server_ctx.peer());
        _server->AddSession<GetPermissionServiceServer>(rlab::ponytal::SysCtrlService::service_full_name(), _udata);
    }

    void OnRequest(const RequestType &request, ResponseType &response) override
    {
        LOG(info) << "GetPermissionServiceServer::OnRequest";
        LOG(info) << "id:" << request.header().seq();
        LOG(info) << "permission:" << request.permission();
        LOG(info) << "arm_ctrl_mode:" << request.arm_ctrl_mode();
        LOG(info) << "mobile_ctrl_mode:" << request.mobile_ctrl_mode();

        /////////////////////////////////////////////////////////////////////////////////////////////
        // set response header
        //
        dt::Rpc::touchTimestamp(response.mutable_header()->mutable_timestamp());
        response.mutable_header()->set_seq(_request.header().seq());
        response.mutable_header()->set_response_code(rlab::common::ResponseHeader_ResponseCode_OK);

        /////////////////////////////////////////////////////////////////////////////////////////////
        // set response state
        //
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

        std::string serviceName = rlab::ponytal::SysCtrlService::service_full_name();
        std::vector<std::pair<std::string, std::unique_ptr<::grpc::Service>>> services;
        services.emplace_back(serviceName, std::make_unique<rlab::ponytal::SysCtrlService::AsyncService>());
        server = std::make_unique<dt::Rpc::Server>(services, address);
        server->AddSession<GetPermissionServiceServer>(serviceName, nullptr);
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