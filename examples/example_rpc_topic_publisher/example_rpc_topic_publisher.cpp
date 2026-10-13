#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcServer.h>
#include <dtCore/src/dtRpc/dtRpcTopicPublisher.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/robot_state_service.grpc.pb.h>

#define NOJ 3
#define NOA 2
#define NOG 2

class RobotStateTopicPublisher : public dt::Rpc::TopicPublisher<rlab::ponytal::GetRobotStateReq, rlab::ponytal::GetRobotStateRsp>
{
    using RequestType = rlab::ponytal::GetRobotStateReq;
    using MessageType = rlab::ponytal::GetRobotStateRsp;

public:
    RobotStateTopicPublisher(
        dt::Rpc::Server *server,
        grpc::Service *service,
        grpc::CompletionQueue *cq_call,
        grpc::ServerCompletionQueue *cq_ntfy,
        void *udata)
        : TopicPublisher<RequestType, MessageType>(server, service, cq_call, cq_ntfy), _udata(udata)
    {
        // pre-allocate joint state
        for (int i = 0; i < NOJ; i++)
        {
            _message.mutable_state()->add_jnt_states();
        }
        // pre-allocate arm(task) state
        for (int i = 0; i < NOA; i++)
        {
            _message.mutable_state()->add_arm_states();
        }
        // pre-allocate gripper state
        for (int i = 0; i < NOG; i++)
        {
            _message.mutable_state()->add_grip_states();
        }

        // prepare async RPC service
        _session_state = SessionState::WAIT_CONNECT;
        LOG(debug) << "NEW RobotStateTopicPublisher(" << this->_id << ") created.";
        (static_cast<rlab::ponytal::RobotStateService::AsyncService *>(service))->RequestGetRobotStateStrm(&_server_ctx, &_request, &_topic_writer, cq_call, cq_ntfy, &_call_context);
    }

    ~RobotStateTopicPublisher()
    {
        LOG(debug) << "RobotStateTopicPublisher(" << this->_id << ") destroyed.";
    }

    void OnConnect() override
    {
        LOG(info) << "RobotStateTopicPublisher connected: " << dt::Rpc::getReadablePeerAddress(_server_ctx.peer());
        _server->AddSession<RobotStateTopicPublisher>(rlab::ponytal::RobotStateService::service_full_name(), _udata);
    }

    void OnRequest(const RequestType &request) override
    {
        LOG(info) << "RobotStateTopicPublisher::OnRequest";
        LOG(info) << "hdr.id:" << request.header().seq();
    }

    void StreamData() override
    {
        if (_session_state == SessionState::WAIT_CONNECT)
            return;
        // if (_session_state != SessionState::READY_TO_WRITE)
        //     return;

        LOG(trace) << "RobotStateTopicPublisher::StreamData(" << _msgSeq << ")";

        /////////////////////////////////////////////////////////////////////////////////////////////
        // update header
        //
        dt::Rpc::touchTimestamp(_message.mutable_header()->mutable_timestamp());
        _message.mutable_header()->set_seq(_msgSeq++);

        /////////////////////////////////////////////////////////////////////////////////////////////
        // update state data
        //

        // joint state
        for (int i = 0; i < NOJ; i++)
        {
            _message.mutable_state()->mutable_jnt_states(i)->set_pos(1.0);
            _message.mutable_state()->mutable_jnt_states(i)->set_vel(2.0);
            _message.mutable_state()->mutable_jnt_states(i)->set_tau(3.0);
        }

        // arm state
        for (int i = 0; i < NOA; i++)
        {
            _message.mutable_state()->mutable_arm_states(i)->mutable_ee_pos()->set_x_m(10.0);
            _message.mutable_state()->mutable_arm_states(i)->mutable_ee_pos()->set_y_m(11.0);
            _message.mutable_state()->mutable_arm_states(i)->mutable_ee_pos()->set_z_m(12.0);
            _message.mutable_state()->mutable_arm_states(i)->mutable_ee_euler_zyx()->set_x_deg(13.0);
            _message.mutable_state()->mutable_arm_states(i)->mutable_ee_euler_zyx()->set_y_deg(14.0);
            _message.mutable_state()->mutable_arm_states(i)->mutable_ee_euler_zyx()->set_z_deg(15.0);
            _message.mutable_state()->mutable_arm_states(i)->set_moving(false);
        }

        // gripper state
        for (int i = 0; i < NOG; i++)
        {
            _message.mutable_state()->mutable_grip_states(i)->set_pos_mm(100.0);
            _message.mutable_state()->mutable_grip_states(i)->set_ang_deg(200.0);
            _message.mutable_state()->mutable_grip_states(i)->set_moving(false);
            _message.mutable_state()->mutable_grip_states(i)->set_contact(true);
        }

        // mobile state
        _message.mutable_state()->mutable_mobile_state()->mutable_vel()->set_vx_mps(50.0);
        _message.mutable_state()->mutable_mobile_state()->mutable_vel()->set_vy_mps(51.0);
        _message.mutable_state()->mutable_mobile_state()->mutable_vel()->set_w_dps(52.0);
        _message.mutable_state()->mutable_mobile_state()->set_moving(false); // (0: 정지, 1: 구동 중)

        /////////////////////////////////////////////////////////////////////////////////////////////
        // publish message
        //
        Publish(_message);
    }

private:
    void *_udata{nullptr};
    uint32_t _msgSeq{0};
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

        std::string serviceName = rlab::ponytal::RobotStateService::service_full_name();
        std::vector<std::pair<std::string, std::unique_ptr<::grpc::Service>>> services;
        services.emplace_back(serviceName, std::make_unique<rlab::ponytal::RobotStateService::AsyncService>());
        server = std::make_unique<dt::Rpc::Server>(services, address);
        server->AddSession<RobotStateTopicPublisher>(serviceName, nullptr);
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