#include "dtCore/src/dtDAQ/grpc/dtDAQManagerGrpc.h"
#include "dtCore/src/dtDAQ/grpc/dtStatePublisherGrpc.hpp"
//#include "dtCore/src/dtLog/dtLog.hpp"

dtCore::dtDAQManagerGrpc DAQ;

int main(int argc, char** argv) 
{
    //google::InitGoogleLogging(argv[0]);
    
    // grpc::EnableDefaultHealthCheckService(true);
    // grpc::reflection::InitProtoReflectionServerBuilderPlugin();


    //DAQ.Initialize();
    //std::shared_ptr<dtCore::dtDataSink> pub = std::make_shared<dtCore::dtStatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> >("RobotState", "0.0.0.0:50051");
    dtCore::dtStatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> pub("RobotState", "0.0.0.0:50051");

    dtproto::robot_msgs::RobotStateTimeStamped msg;
    for (int ji = 0; ji < 3; ji++) {
        msg.mutable_state()->add_joint_state();
    }
    std::function<double(void)> data_gen = ([]() -> double { return (double)rand() / (double)RAND_MAX; });
    
    while (true) {
        std::cout << "(type \'p\' to publish message or \'q\' to quit) >";
        std::string cmd;
        std::cin >> cmd;
        if (cmd == "q" || cmd == "quit") {
            break;
        }
        else {
            for (uint32_t seq = 0; seq < 1000; seq++) {
                std::cout << seq << std::endl;

                msg.mutable_header()->set_seq(seq);
                msg.mutable_state()->mutable_base_pose()->mutable_position()->set_x(data_gen());
                msg.mutable_state()->mutable_base_pose()->mutable_orientation()->set_w(data_gen());
                msg.mutable_state()->mutable_base_velocity()->mutable_linear()->set_x(data_gen());
                msg.mutable_state()->mutable_base_velocity()->mutable_angular()->set_x(data_gen());
                for (int ji = 0; ji < 3; ji++) {
                    msg.mutable_state()->mutable_joint_state(ji)->set_position(data_gen());
                    msg.mutable_state()->mutable_joint_state(ji)->set_velocity(data_gen());
                    msg.mutable_state()->mutable_joint_state(ji)->set_acceleration(data_gen());
                    msg.mutable_state()->mutable_joint_state(ji)->set_torque(data_gen());
                }
                pub.Publish(msg);
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }

    //DAQ.Terminate();

    return 0;
}