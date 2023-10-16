#include "dtCore/src/dtDAQ/grpc/dtStatePublisherGrpc.hpp"



int main(int argc, char** argv) 
{
    //google::InitGoogleLogging(argv[0]);

    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();



    dtCore::dtStatePublisherGrpc<dtproto::robot_msgs::RobotStateTimeStamped> pub("RobotState", "0.0.0.0:50051");

    dtproto::robot_msgs::RobotStateTimeStamped msg;
    for (int ji = 0; ji < 3; ji++) {
        msg.mutable_state()->add_joint_state();
    }
    std::function<double(void)> data_gen = ([]() -> double { return (double)rand() / (double)RAND_MAX; });
    uint32_t seq = 0;
    while (true)
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
    }

    return 0;
}
