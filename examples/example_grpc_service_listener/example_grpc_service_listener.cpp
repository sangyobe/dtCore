#include <dtCore/src/dtDAQ/grpc/dtServiceListenerGrpc.hpp>
#include <dtCore/src/dtLog/dtLog.h>

/////////////////////////////////////////////////////////////////////////
// OnControlCmd (Rpc service call handler)
//
//     rpc Command(robot_msgs.ControlCmd) returns (std_msgs.Response);
//
class OnControlCmd : public dtCore::dtServiceListenerGrpc::Session 
{
    using CallState = typename dtCore::dtServiceListenerGrpc::Session::CallState;
    using ServiceType = dtproto::dtService::AsyncService;
public:
    OnControlCmd(dtCore::dtServiceListenerGrpc* server, grpc::Service* service, grpc::ServerCompletionQueue* cq, void* udata = nullptr)
    : dtCore::dtServiceListenerGrpc::Session(server, service, cq, udata), _responder(&_ctx) {
        _call_state = CallState::WAIT_CONNECT;
        (static_cast<ServiceType*>(_service))->RequestCommand(&(_ctx), &_request, &_responder, _cq, _cq, this);
        LOG(info) << "Command[" << _id << "] Wait for new service call...";
    }
    ~OnControlCmd() {
        // LOG(info) << "OnControlCmd session deleted."; // Do not output log here. It might be after LOG system has been destroyed.
    }
    bool OnCompletionEvent(bool ok) override {
        if (_call_state == CallState::FINISHED) {
            return false;
        }
        else if (ok) {
            if (_call_state == CallState::WAIT_CONNECT) {
                LOG(info) << "Command[" << _id << "] NEW service call.";

                _server->template AddSession<OnControlCmd >();
                {
                    std::lock_guard<std::mutex> lock(_proc_mtx);

                    LOG(trace) << "Command[" << _id << "]\tcmd_mode=" << _request.cmd_mode() << " , arg=" << _request.arg();

                    _response.set_rtn(0);
                    _response.set_msg("success");

                    _call_state = CallState::WAIT_FINISH;
                    _responder.Finish(_response, grpc::Status::OK, this);
                }
            } 
            else if (_call_state == CallState::WAIT_FINISH) {
                LOG(info) << "Command[" << _id << "] Finalize service.";
                // _call_state = CallState::FINISHED;
                // _server->RemoveSession(_id);
                return false;
            }
            else {
                GPR_ASSERT(false && "Invalid Session Status.");
                LOG(err) << "Command[" << _id << "] Invalid session status (" << static_cast<int>(_call_state) << ")";
                return false;
            }
        }
        else {
            if (_call_state == CallState::WAIT_CONNECT) {
                LOG(err) << "Command[" << _id << "] Session has been shut down before receiving a matching request.";
                return false;
            }
            else {
                std::lock_guard<std::mutex> lock(_proc_mtx);
                _responder.Finish(_response, grpc::Status::CANCELLED, this);
                _call_state == CallState::WAIT_FINISH;
            }
        }
        return true;
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
    using CallState = typename dtCore::dtServiceListenerGrpc::Session::CallState;
    using ServiceType = dtproto::dtService::AsyncService;
public:
    OnQueryRobotInfo(dtCore::dtServiceListenerGrpc* server, grpc::Service* service, grpc::ServerCompletionQueue* cq, void* udata = nullptr)
    : dtCore::dtServiceListenerGrpc::Session(server, service, cq, udata), _responder(&_ctx) {
        _call_state = CallState::WAIT_CONNECT;
        (static_cast<ServiceType*>(_service))->RequestQueryRobotInfo(&_ctx, &_request, &_responder, _cq, _cq, this);
        LOG(info) << "QueryRobotInfo[" << _id << "] Waiting for new service call...";
    }
    ~OnQueryRobotInfo() {
        // LOG(info) << "OnQueryRobotInfo session deleted."; // Do not output log here. It might be after LOG system has been destroyed.
    }
    bool OnCompletionEvent(bool ok) override {
        if (_call_state == CallState::FINISHED) {
            return false;
        }
        else if (ok) {
            if (_call_state == CallState::WAIT_CONNECT) {
                LOG(info) << "QueryRobotInfo[" << _id << "] NEW service call.";

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

                    _call_state = CallState::WAIT_FINISH;
                    _responder.Finish(_response, grpc::Status::OK, this);
                }
            } 
            else if (_call_state == CallState::WAIT_FINISH) {
                LOG(info) << "QueryRobotInfo[" << _id << "] Finalize service.";
                // _call_state = CallState::FINISHED;
                // _server->RemoveSession(_id);
                return false;
            }
            else {
                GPR_ASSERT(false && "Invalid Session Status.");
                LOG(err) << "QueryRobotInfo[" << _id << "] Invalid session status (" << static_cast<int>(_call_state) << ")";
                return false;
            }
        }
        else {
            if (_call_state == CallState::WAIT_CONNECT) {
                LOG(err) << "QueryRobotInfo[" << _id << "] Session has been shut down before receiving a matching request.";
                return false;
            }
            else {
                std::lock_guard<std::mutex> lock(_proc_mtx);
                _responder.Finish(_response, grpc::Status::CANCELLED, this);
                _call_state == CallState::WAIT_FINISH;
            }
        }
        return true;
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
