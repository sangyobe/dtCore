#include "dtCore/src/dtDAQ/grpc/dtDAQManagerGrpc.h"
#include "dtCore/src/dtDAQ/grpc/dtServiceListenerGrpc.hpp"
#include "dtCore/src/dtLog/dtLog.h"

dtCore::dtDAQManagerGrpc DAQ;

int main(int argc, char** argv) 
{
    dtCore::dtLog::Initialize("grpc_state_pub", "logs/grpc_service_listener.txt");
    dtCore::dtLog::SetLogLevel(dtCore::dtLog::LogLevel::trace);

    DAQ.Initialize();

    dtCore::dtServiceListenerGrpc listener("0.0.0.0:50052");
    listener.AddSession<dtCore::OnControlCmd>();

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
    while (bRun.load()) {
        DAQ.Update();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    chk_key.join();

    listener.Stop();

    DAQ.Terminate();

    dtCore::dtLog::Terminate();

    return 0;
}
