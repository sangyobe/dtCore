#include "dtCore/src/dtDAQ/grpc/dtDAQManagerGrpc.h"
#include "dtCore/src/dtDAQ/grpc/dtStatePublisherGrpc.hpp"
#include "dtCore/src/dtLog/dtLog.h"
#include "dtProto/robot_msgs/ArbitraryState.pb.h"
#include "dtProto/robot_msgs/RobotState.pb.h"

dt::DAQ::DAQManagerGrpc DAQ;

std::function<double(void)> data_gen = ([]() -> double { return (double)rand() / (double)RAND_MAX; });

int main(int argc, char** argv) 
{
    //google::InitGoogleLogging(argv[0]);

    dt::Log::Initialize("grpc_state_pub", "logs/grpc_state_pub.txt");
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);

    DAQ.Initialize();
    // grpc::EnableDefaultHealthCheckService(true);
    // grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    

    std::atomic<bool> bRun;
    bRun.store(true);

    std::thread proc_pub_robot_state = std::thread([&bRun]() {
        //std::shared_ptr<dt::DAQ::DataSink> pub = std::make_shared<dt::DAQ::StatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> >("RobotState", "0.0.0.0:50051");
        dt::DAQ::StatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> pub("RobotState", "0.0.0.0:50053");
        dtproto::robot_msgs::RobotStateTimeStamped msg;

        for (int ji = 0; ji < 3; ji++)
        {
            msg.mutable_state()->add_joint_state();
        }

        uint32_t seq = 0;
        while (bRun.load())
        {
            msg.mutable_header()->set_seq(seq++);
            msg.mutable_state()->mutable_base_pose()->mutable_position()->set_x(data_gen());
            msg.mutable_state()->mutable_base_pose()->mutable_orientation()->set_w(data_gen());
            msg.mutable_state()->mutable_base_velocity()->mutable_linear()->set_x(data_gen());
            msg.mutable_state()->mutable_base_velocity()->mutable_angular()->set_x(data_gen());
            for (int ji = 0; ji < 3; ji++)
            {
                msg.mutable_state()->mutable_joint_state(ji)->set_position(data_gen());
                msg.mutable_state()->mutable_joint_state(ji)->set_velocity(data_gen());
                msg.mutable_state()->mutable_joint_state(ji)->set_acceleration(data_gen());
                msg.mutable_state()->mutable_joint_state(ji)->set_torque(data_gen());
            }
            pub.Publish(msg);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });

    std::thread proc_pub_arbitrary_state = std::thread([&bRun]() {
        dt::DAQ::StatePublisherGrpc<dtproto::robot_msgs::ArbitraryStateTimeStamped> pub("ArbitraryState", "0.0.0.0:50052");
        dtproto::robot_msgs::ArbitraryStateTimeStamped msg;

        for (int ji = 0; ji < 2; ji++)
        {
            msg.mutable_state()->add_data(0.0);
        }

        uint32_t seq = 0;
        while (bRun.load())
        {
            msg.mutable_header()->set_seq(seq++);
            for (int ji = 0; ji < 2; ji++)
            {
                msg.mutable_state()->set_data(ji, data_gen());
            }
            pub.Publish(msg);
            
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    
    proc_pub_robot_state.join();
    proc_pub_arbitrary_state.join();
    DAQ.Terminate();
    dt::Log::Terminate();

    return 0;
}
