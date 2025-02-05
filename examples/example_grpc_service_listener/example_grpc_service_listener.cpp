#include <dtCore/src/dtDAQ/grpc/dtServiceListenerGrpc.hpp>
#include <dtCore/src/dtLog/dtLog.h>

/////////////////////////////////////////////////////////////////////////
// OnControlCmd (Rpc service call handler)
//
//     rpc Command(robot_msgs.ControlCmd) returns (std_msgs.Response);
//
class OnControlCmd : public dt::DAQ::ServiceListenerGrpc::Session
{
    using CallState = typename dt::DAQ::ServiceListenerGrpc::Session::CallState;
    using ServiceType = dtproto::dtService::AsyncService;
public:
    OnControlCmd(dt::DAQ::ServiceListenerGrpc *server, grpc::Service *service, grpc::ServerCompletionQueue *cq, void *udata = nullptr)
        : dt::DAQ::ServiceListenerGrpc::Session(server, service, cq, udata), _responder(&_ctx)
    {
        _call_state = CallState::WAIT_CONNECT;
        (static_cast<ServiceType*>(_service))->RequestCommand(&(_ctx), &_request, &_responder, _cq, _cq, this);
        LOG(info) << "Command[" << _id << "] Wait for new service call...";
    }
    ~OnControlCmd() {
        // LOG(info) << "OnControlCmd session deleted."; // Do not output log here. It might be after LOG system has been destroyed.
    }
    bool OnCompletionEvent(bool ok) override {
        if (_call_state == CallState::FINISHED)
        {
            return true;
        }
        else if (_call_state == CallState::WAIT_FINISH)
        {
            LOG(info) << "Command[" << _id << "] Finalize service.";
            // _call_state = CallState::FINISHED;
            // _server->RemoveSession(_id);
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
                    LOG(trace) << "Command[" << _id << "] Finish()";
                    _responder.Finish(_response, grpc::Status::OK, this);
                }
            }
            else {
                LOG(err) << "Command[" << _id << "] Invalid session status (" << static_cast<int>(_call_state) << ")";
                ABSL_ASSERT(false && "Invalid Session Status.");
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
                LOG(trace) << "Command[" << _id << "] Finish()";
                _responder.Finish(_response, grpc::Status::CANCELLED, this);
                _call_state = CallState::WAIT_FINISH;
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
//     rpc RequestRobotInfo(google.protobuf.Empty) returns (robot_msgs.RobotInfo);
//
class OnQueryRobotInfo : public dt::DAQ::ServiceListenerGrpc::Session
{
    using CallState = typename dt::DAQ::ServiceListenerGrpc::Session::CallState;
    using ServiceType = dtproto::dtService::AsyncService;
public:
    OnQueryRobotInfo(dt::DAQ::ServiceListenerGrpc *server, grpc::Service *service, grpc::ServerCompletionQueue *cq, void *udata = nullptr)
        : dt::DAQ::ServiceListenerGrpc::Session(server, service, cq, udata), _responder(&_ctx)
    {
        _call_state = CallState::WAIT_CONNECT;
        (static_cast<ServiceType *>(_service))->RequestRequestRobotInfo(&_ctx, &_request, &_responder, _cq, _cq, this);
        LOG(info) << "RequestRobotInfo[" << _id << "] Waiting for new service call...";
    }
    ~OnQueryRobotInfo() {
        // LOG(info) << "OnQueryRobotInfo session deleted."; // Do not output log here. It might be after LOG system has been destroyed.
    }
    bool OnCompletionEvent(bool ok) override {
        if (_call_state == CallState::FINISHED)
        {
            return true;
        }
        else if (_call_state == CallState::WAIT_FINISH)
        {
            LOG(info) << "RequestRobotInfo[" << _id << "] Finalize service.";
            // _call_state = CallState::FINISHED;
            // _server->RemoveSession(_id);
            return false;
        }
        else if (ok) {
            if (_call_state == CallState::WAIT_CONNECT) {
                LOG(info) << "RequestRobotInfo[" << _id << "] NEW service call.";

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
                    LOG(trace) << "RequestRobotInfo[" << _id << "] Finish()";
                    _responder.Finish(_response, grpc::Status::OK, this);
                }
            }
            else {
                LOG(err) << "RequestRobotInfo[" << _id << "] Invalid session status (" << static_cast<int>(_call_state) << ")";
                ABSL_ASSERT(false && "Invalid Session Status.");
                return false;
            }
        }
        else {
            if (_call_state == CallState::WAIT_CONNECT) {
                LOG(err) << "RequestRobotInfo[" << _id << "] Session has been shut down before receiving a matching request.";
                return false;
            }
            else {
                std::lock_guard<std::mutex> lock(_proc_mtx);
                LOG(trace) << "RequestRobotInfo[" << _id << "] Finish()";
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
    dt::Log::Initialize("grpc_service_listener"); //, "logs/grpc_service_listener.txt");
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);

    dt::DAQ::ServiceListenerGrpc listener(
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

    dt::Log::Terminate(); // flush all log messages
    return 0;
}
