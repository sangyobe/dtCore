#include "dtCore/src/dtDAQ/grpc/dtStateSubscriberGrpc.hpp"
#include "dtProto/robot_msgs/RobotState.pb.h"
#include "dtProto/robot_msgs/AnonState.pb.h"
#include "dtCore/src/dtLog/dtLog.h"

int main(int argc, char** argv) 
{
    dtCore::dtLog::Initialize("grpc_state_sub");
    dtCore::dtLog::SetLogLevel(dtCore::dtLog::LogLevel::trace);

    std::function<void(dtproto::robot_msgs::RobotStateTimeStamped&)> handler_robot_state = [](dtproto::robot_msgs::RobotStateTimeStamped& msg) {
        LOG(info) << "handler_robot_state Got a new message. Type = " << msg.state().GetTypeName();

        if (msg.state().GetTypeName() != "dtproto.robot_msgs.RobotState") {
            return;
        }

        LOG(trace) << "  base_pose.position.x = "          << msg.state().base_pose().position().x();
        LOG(trace) << "  base_pose.orientation.w = "       << msg.state().base_pose().orientation().w();
        LOG(trace) << "  base_velocity.linear.x = "        << msg.state().base_velocity().linear().x();
        LOG(trace) << "  base_velocity.angular.x = "       << msg.state().base_velocity().angular().x();
        for (int ji = 0; ji < 3; ji++) {
            LOG(trace) << "    joint[" << ji << "].position = "     << msg.state().joint_state(ji).position();
            LOG(trace) << "    joint[" << ji << "].velocity = "     << msg.state().joint_state(ji).velocity();
            LOG(trace) << "    joint[" << ji << "].acceleration = " << msg.state().joint_state(ji).acceleration();
            LOG(trace) << "    joint[" << ji << "].torque = "       << msg.state().joint_state(ji).torque();
        }
    };
    std::unique_ptr<dtCore::dtStateSubscriberGrpc<dtproto::robot_msgs::RobotStateTimeStamped>> sub_robot_state
        = std::make_unique<dtCore::dtStateSubscriberGrpc<dtproto::robot_msgs::RobotStateTimeStamped>>("RobotState", "0.0.0.0:50053");
    sub_robot_state->RegMessageHandler(handler_robot_state);

    std::function<void(dtproto::robot_msgs::AnonStateTimeStamped&)> handler_anon_state = [](dtproto::robot_msgs::AnonStateTimeStamped& msg) {
        LOG(info) << "handler_anon_state Got a new message. Type = " << msg.state().GetTypeName();

        if (msg.state().GetTypeName() != "dtproto.std_msgs.PackedDouble") {
            return;
        }

        for (int ji = 0; ji < msg.state().data_size(); ji++) {
            LOG(trace) << "  data[" << ji << "] = " << msg.state().data(ji);
        }
    };
    std::unique_ptr<dtCore::dtStateSubscriberGrpc<dtproto::robot_msgs::AnonStateTimeStamped>> sub_anon_state
        = std::make_unique<dtCore::dtStateSubscriberGrpc<dtproto::robot_msgs::AnonStateTimeStamped>>("AnonState", "0.0.0.0:50052");
    sub_anon_state->RegMessageHandler(handler_anon_state);



    std::atomic<bool> bRun;
    bRun.store(true);

    std::thread proc_reconnector = std::thread([&bRun, &sub_robot_state, &sub_anon_state] () {

        while (bRun.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            if (!sub_robot_state->IsRunning()) {
                std::cout << "Reconnect to robot state publisher..." << std::endl;
                sub_robot_state->Reconnect();
            }

            if (!sub_anon_state->IsRunning()) {
                std::cout << "Reconnect to anonymous state publisher..." << std::endl;
                sub_anon_state->Reconnect();
            }
        }
    });

    while (bRun.load()) {
        std::cout << "(type \'q\' to quit) >\n";
        std::string cmd;
        std::cin >> cmd;
        if (cmd == "q" || cmd == "quit") {
            bRun = false;
        }
    }

    proc_reconnector.join();
    dtCore::dtLog::Terminate();

    return 0;
}