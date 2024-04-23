#include "dtCore/src/dtDAQ/mcap/dtDataSinkPBMcap.hpp"
#include "dtProto/robot_msgs/RobotState.pb.h"
#include "dtProto/robot_msgs/ArbitraryState.pb.h"
#include "dtCore/src/dtLog/dtLog.h"

std::function<double(void)> data_gen = ([]() -> double { return (double)rand() / (double)RAND_MAX; });

int main(int argc, char** argv) 
{
    dtCore::dtLog::Initialize("state_filesink_mcap", "logs/filesink_mcap.txt");
    dtCore::dtLog::SetLogLevel(dtCore::dtLog::LogLevel::trace);

    std::atomic<bool> bRun;
    bRun.store(true);

    std::thread proc_pub_robot_state = std::thread([&bRun] () {
        dtCore::dtDataSinkPBMcap<dtproto::robot_msgs::RobotStateTimeStamped> pub("RobotState", "data/robot_state.mcap");
        dtproto::robot_msgs::RobotStateTimeStamped msg;

        for (int ji = 0; ji < 3; ji++) {
            msg.mutable_state()->add_joint_state();
        }

        uint32_t seq = 0;
        while (bRun.load())
        {
            struct timespec tp;
            clock_gettime(CLOCK_REALTIME, &tp);
            msg.mutable_header()->mutable_time_stamp()->set_seconds(tp.tv_sec);
            msg.mutable_header()->mutable_time_stamp()->set_nanos(tp.tv_nsec);
            msg.mutable_header()->set_seq(seq++);
            double ts = (double)tp.tv_sec + 1e-9*(double)tp.tv_nsec;
            msg.mutable_state()->mutable_base_pose()->mutable_position()->set_x(sin(2.0*3.14*0.5*ts) + 0.1 * data_gen());
            msg.mutable_state()->mutable_base_pose()->mutable_orientation()->set_w(data_gen());
            msg.mutable_state()->mutable_base_velocity()->mutable_linear()->set_x(cos(2.0*3.14*0.5*ts) + 0.3 * data_gen());
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
    });

    std::thread proc_pub_arbitrary_state = std::thread([&bRun] () {
        dtCore::dtDataSinkPBMcap<dtproto::robot_msgs::ArbitraryStateTimeStamped> pub("ArbitraryState", "data/arbitrary_state.mcap");
        dtproto::robot_msgs::ArbitraryStateTimeStamped msg;

        for (int ji = 0; ji < 2; ji++) {
            msg.mutable_state()->add_data(0.0);
        }

        uint32_t seq = 0;
        while (bRun.load())
        {
            msg.mutable_header()->set_seq(seq++);
            for (int ji = 0; ji < 2; ji++) {
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
    dtCore::dtLog::Terminate();

    return 0;
}
