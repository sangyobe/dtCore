#include "dtCore/src/dtDAQ/grpc/dtServiceListenerGrpc.hpp"
#include "dtCore/src/dtLog/dtLog.h"

/////////////////////////////////////////////////////////////////////////
// OnControlCmd (Rpc service call handler)
//
//     rpc Command(robot_msgs.ControlCmd) returns (std_msgs.Response);
//
class OnControlCmd : public dtCore::dtServiceListenerGrpc::Session 
{
    using SessionStatus = typename dtCore::dtServiceListenerGrpc::Session::SessionStatus;
    using ServiceType = dtproto::dtService::AsyncService;
public:
    OnControlCmd(dtCore::dtServiceListenerGrpc* server, grpc::Service* service, grpc::ServerCompletionQueue* cq, void* udata = nullptr)
    : dtCore::dtServiceListenerGrpc::Session(server, service, cq, udata), _responder(&_ctx) {
        LOG(info) << "NEW OnControlCmd session created.";
        _status = SessionStatus::WAIT_CONNECT;
        (static_cast<ServiceType*>(_service))->RequestCommand(&(_ctx), &_request, &_responder, _cq, _cq, this);
        LOG(info) << "Wait for new dtService::Command() service call...";
    }
    ~OnControlCmd() {
        // LOG(info) << "OnControlCmd session deleted."; // Do not output log here. It might be after LOG system has been destroyed.
    }
    void OnCompletionEvent() {
        LOG(info) << "OnControlCmd: " << "OnCompletionEvent";
        if (_status == SessionStatus::WAIT_CONNECT) {
            LOG(info) << "NEW dtService::Command() service call.";

            _server->template AddSession<OnControlCmd >();
            {
                std::lock_guard<std::mutex> lock(_proc_mtx);

                std::cout << "Recv! cmd_mode=" << _request.cmd_mode() << " , arg=" << _request.arg() << std::endl;

                _response.set_rtn(0);
                _response.set_msg("success");

                _status = SessionStatus::WAIT_FINISH;
                _responder.Finish(_response, grpc::Status::OK, this);
            }
        } 
        else if (_status == SessionStatus::WAIT_FINISH) {
            LOG(info) << "Finalize dtService::Command() service.";
            //_status = SessionStatus::FINISHED;
            _server->RemoveSession(_id);
        }
        else {
            GPR_ASSERT(false && "Invalid Session Status.");
            LOG(err) << "Invalid session status (" << static_cast<int>(_status) << ")";
        }
    }

private:
    ::dtproto::robot_msgs::ControlCmd _request;
    ::dtproto::std_msgs::Response _response;
    ::grpc::ServerAsyncResponseWriter<::dtproto::std_msgs::Response> _responder;
};

/////////////////////////////////////////////////////////////////////////
// OnQueryRobotInfo (Rpc service call handler)
//
//     rpc QueryRobotInfo(google.protobuf.Empty) returns (robot_msgs.RobotInfo);
//
class OnQueryRobotInfo : public dtCore::dtServiceListenerGrpc::Session 
{
    using SessionStatus = typename dtCore::dtServiceListenerGrpc::Session::SessionStatus;
    using ServiceType = dtproto::dtService::AsyncService;
public:
    OnQueryRobotInfo(dtCore::dtServiceListenerGrpc* server, grpc::Service* service, grpc::ServerCompletionQueue* cq, void* udata = nullptr)
    : dtCore::dtServiceListenerGrpc::Session(server, service, cq, udata), _responder(&_ctx) {
        LOG(info) << "NEW OnQueryRobotInfo session created.";
        _status = SessionStatus::WAIT_CONNECT;
        (static_cast<ServiceType*>(_service))->RequestQueryRobotInfo(&_ctx, &_request, &_responder, _cq, _cq, this);
        LOG(info) << "Wait for new dtService::QueryRobotInfo() service call...";
    }
    ~OnQueryRobotInfo() {
        // LOG(info) << "OnQueryRobotInfo session deleted."; // Do not output log here. It might be after LOG system has been destroyed.
    }
    void OnCompletionEvent() {
        LOG(info) << "OnQueryRobotInfo: " << "OnCompletionEvent";
        if (_status == SessionStatus::WAIT_CONNECT) {
            LOG(info) << "NEW dtService::QueryRobotInfo() service call.";

            _server->template AddSession<OnQueryRobotInfo >();
            {
                std::lock_guard<std::mutex> lock(_proc_mtx);

                _response.set_name("QuadIP");
                _response.set_version("v0.1");
                _response.set_author("HMC");
                _response.set_description("");
                _response.set_serial("HMC-8-001");
                _response.set_type(8);
                _response.set_id(1);
                _response.set_dof(12);

                _status = SessionStatus::WAIT_FINISH;
                _responder.Finish(_response, grpc::Status::OK, this);
            }
        } 
        else if (_status == SessionStatus::WAIT_FINISH) {
            LOG(info) << "Finalize dtService::QueryRobotInfo() service.";
            //_status = dtCore::dtServiceListenerGrpc::SessionStatus::FINISHED;
            _server->RemoveSession(_id);
        }
        else {
            GPR_ASSERT(false && "Invalid Session Status.");
            LOG(err) << "Invalid session status (" << static_cast<int>(_status) << ")";
        }
    }

private:
    ::google::protobuf::Empty _request;
    ::dtproto::robot_msgs::RobotInfo _response;
    ::grpc::ServerAsyncResponseWriter<::dtproto::robot_msgs::RobotInfo> _responder;
};


/////////////////////////////////////////////////////////////////////////
// main
//
int main(int argc, char** argv) 
{
    dtCore::dtLog::Initialize("grpc_service_listener"); //, "logs/grpc_service_listener.txt");
    dtCore::dtLog::SetLogLevel(dtCore::dtLog::LogLevel::trace);

    dtCore::dtServiceListenerGrpc listener(
        std::make_unique<dtproto::dtService::AsyncService>(), 
        "0.0.0.0:50052");

    listener.template AddSession<OnControlCmd>(nullptr);
    listener.template AddSession<OnQueryRobotInfo>(nullptr);

    std::atomic<bool> bRun{true};
    while (bRun.load()) {
        std::cout << "(type \'q\' to quit) >\n";
        std::string cmd;
        std::cin >> cmd;
        if (cmd == "q" || cmd == "quit") {
            bRun = false;
        }
    }
    listener.Stop();

    dtCore::dtLog::Terminate(); // flush all log messages
    return 0;
}
