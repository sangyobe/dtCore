#include "dtCore/src/dtDAQ/grpc/dtDAQManagerGrpc.h"
#include "dtCore/src/dtDAQ/grpc/dtStatePublisherGrpc.hpp"
#include "dtCore/src/dtLog/dtLog.h"

dtCore::dtDAQManagerGrpc DAQ;

int main(int argc, char** argv) 
{
    //google::InitGoogleLogging(argv[0]);
    
    // grpc::EnableDefaultHealthCheckService(true);
    // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

    dtCore::dtLog::Initialize("grpc_state_pub", "logs/grpc_state_pub.txt");
    dtCore::dtLog::SetLogLevel(dtCore::dtLog::LogLevel::trace);


    //DAQ.Initialize();
    //std::shared_ptr<dtCore::dtDataSink> pub = std::make_shared<dtCore::dtStatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> >("RobotState", "0.0.0.0:50051");
    dtCore::dtStatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> pub("RobotState", "0.0.0.0:50051");

    dtproto::robot_msgs::RobotStateTimeStamped msg;
    for (int ji = 0; ji < 3; ji++) {
        msg.mutable_state()->add_joint_state();
    }
    std::function<double(void)> data_gen = ([]() -> double { return (double)rand() / (double)RAND_MAX; });
    

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

    uint32_t seq = 0;
    while (bRun.load())
    {
        msg.mutable_header()->set_seq(seq++);
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
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    chk_key.join();

    //DAQ.Terminate();

    dtCore::dtLog::Terminate();

    return 0;
}
