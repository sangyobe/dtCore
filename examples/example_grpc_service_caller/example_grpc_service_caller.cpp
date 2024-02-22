#include "dtCore/src/dtDAQ/grpc/dtServiceCallerGrpc.hpp"
#include "dtCore/src/dtLog/dtLog.h"

using ServiceType = dtproto::dtService;

/////////////////////////////////////////////////////////////////////////
// OnQueryRobotInfo (Rpc service call handler)
//
//     rpc QueryRobotInfo(google.protobuf.Empty) returns (robot_msgs.RobotInfo);
//
class OnQueryRobotInfo : public dtCore::dtServiceCallerGrpc<ServiceType>::Call {
  using CallState =
      typename dtCore::dtServiceCallerGrpc<ServiceType>::Call::CallState;

public:
  OnQueryRobotInfo(ServiceType::Stub *stub, grpc::CompletionQueue *cq,
                   void *udata = nullptr)
      : dtCore::dtServiceCallerGrpc<ServiceType>::Call(stub, cq, udata) {
    LOG(info) << "NEW OnQueryRobotInfo session created.";
    _responder =
        stub->PrepareAsyncQueryRobotInfo(&(this->_ctx), _request, this->_cq);
    _responder->StartCall();
    _responder->Finish(&_response, &(this->_status), (void *)this);
    this->_call_state = CallState::WAIT_FINISH;
    LOG(info) << "Wait for response for QueryRobotInfo() service call...";
  }

  ~OnQueryRobotInfo() {
    // LOG(info) << "OnQueryRobotInfo session deleted."; // Do not output log
    // here. It might be after LOG system has been destroyed.
  }
  bool OnCompletionEvent() {
    LOG(info) << "OnQueryRobotInfo: OnCompletionEvent";
    if (this->_call_state == CallState::WAIT_FINISH) {
      LOG(info) << "Get response for QueryRobotInfo() service call.";
      {
        std::lock_guard<std::mutex> lock(this->_proc_mtx);

        std::cout << "name : " << _response.name() << std::endl;
        std::cout << "version : " << _response.version() << std::endl;
        std::cout << "author : " << _response.author() << std::endl;
        std::cout << "description : " << _response.description() << std::endl;
        std::cout << "serial(id) : " << _response.serial() << "("
                  << _response.id() << ")" << std::endl;
        std::cout << "type : " << _response.type() << std::endl;
        std::cout << "dof : " << _response.dof() << std::endl;

        _call_state = CallState::FINISHED;
      }
      return false; // remove this call
    } else {
      GPR_ASSERT(false && "Invalid Call State.");
      LOG(err) << "Invalid call state (" << static_cast<int>(_call_state)
               << ")";
      return false;
    }
  }

private:
  ::google::protobuf::Empty _request;
  ::dtproto::robot_msgs::RobotInfo _response;
  std::unique_ptr<::grpc::ClientAsyncResponseReader<::dtproto::robot_msgs::RobotInfo>> _responder;
};

class RpcClient : public dtCore::dtServiceCallerGrpc<ServiceType> {
public:
  RpcClient(const std::string &server_address)
      : dtCore::dtServiceCallerGrpc<ServiceType>(server_address) {}
};

/////////////////////////////////////////////////////////////////////////
// main
//
int main(int argc, char **argv) {
  // initialize RPC client
  std::unique_ptr<RpcClient> rpcClient =
      std::make_unique<RpcClient>("localhost:50052");

  std::cout << "> ControlCmd() call ...\n";
  rpcClient->template StartCall<OnQueryRobotInfo>(nullptr);

  std::atomic<bool> bRun{true};
  while (bRun.load()) {
    std::cout << "(type \'q\' to quit) >\n";
    std::string cmd;
    std::cin >> cmd;
    if (cmd == "q" || cmd == "quit") {
      bRun = false;
    }
  }

  return 0;
}
