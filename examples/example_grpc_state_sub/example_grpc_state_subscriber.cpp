#include "dtCore/src/dtDAQ/grpc/dtStateSubscriberGrpc.hpp"
//#include "dtCore/src/dtLog/dtLog.h"

int main(int argc, char** argv) 
{
    //google::InitGoogleLogging(argv[0]);
    //dtCore::dtLog::Initialize(argv[0]);

    std::function<void(dtproto::robot_msgs::RobotStateTimeStamped&)> handler = [](dtproto::robot_msgs::RobotStateTimeStamped& msg) {
        // LOG(info) << "Got a new message. Type = " << msg.state().GetTypeName();

        // if (msg.state().type_url() != "type.googleapis.com/dtproto.robot_msgs.RobotState") {
        //     return;
        // }

        std::cout << "  base_pose.position.x = "          << msg.state().base_pose().position().x()         << std::endl;
        std::cout << "  base_pose.orientation.w = "       << msg.state().base_pose().orientation().w()      << std::endl;
        std::cout << "  base_velocity.linear.x = "        << msg.state().base_velocity().linear().x()       << std::endl;
        std::cout << "  base_velocity.angular.x = "       << msg.state().base_velocity().angular().x()      << std::endl;
        for (int ji = 0; ji < 3; ji++) {
            std::cout << "    joint[" << ji << "].position = "     << msg.state().joint_state(ji).position()     << std::endl;
            std::cout << "    joint[" << ji << "].velocity = "     << msg.state().joint_state(ji).velocity()     << std::endl;
            std::cout << "    joint[" << ji << "].acceleration = " << msg.state().joint_state(ji).acceleration() << std::endl;
            std::cout << "    joint[" << ji << "].torque = "       << msg.state().joint_state(ji).torque()       << std::endl;
        }

    };



    std::unique_ptr<dtCore::dtStateSubscriberGrpc<dtproto::robot_msgs::RobotStateTimeStamped>> sub
        = std::make_unique<dtCore::dtStateSubscriberGrpc<dtproto::robot_msgs::RobotStateTimeStamped>>("RobotState", "0.0.0.0:50051");
    sub->RegMessageHandler(handler);


    std::atomic<bool> bRun;
    bRun.store(true);
    std::thread chk_key = std::thread([&bRun] () {
        while (true) {
            std::cout << "(type \'q\' to quit) >\n";
            std::string cmd;
            std::cin >> cmd;
            if (cmd == "q" || cmd == "quit") {
                bRun = false;
                return;
            }
        }
    });

    while (bRun.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if (!sub->IsRunning()) {
            std::cout << "Reconnect..." << std::endl;
            sub->Reconnect();
        }
    }
    chk_key.join();

    return 0;
}