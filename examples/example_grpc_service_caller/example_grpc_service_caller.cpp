#include <dtCore/src/dtDAQ/grpc/dtServiceCallerGrpc.hpp>
#include <dtCore/src/dtLog/dtLog.h>

using ServiceType = dtproto::dtService;

/////////////////////////////////////////////////////////////////////////
// OnQueryRobotInfo (Rpc service call handler)
//
//     rpc RequestRobotInfo(google.protobuf.Empty) returns (robot_msgs.RobotInfo);
//
class OnQueryRobotInfo : public dt::DAQ::ServiceCallerGrpc<ServiceType>::Call
{
    using CallState =
        typename dt::DAQ::ServiceCallerGrpc<ServiceType>::Call::CallState;

public:
    OnQueryRobotInfo(ServiceType::Stub *stub, grpc::CompletionQueue *cq,
                     void *udata = nullptr)
        : dt::DAQ::ServiceCallerGrpc<ServiceType>::Call(stub, cq, udata)
    {
        LOG(info) << "RequestRobotInfo[" << this->_id << "] NEW call.";
        _call_id = *(int *)(udata);
        _responder =
            stub->PrepareAsyncRequestRobotInfo(&(this->_ctx), _request, this->_cq);
        _responder->StartCall();
        _responder->Finish(&_response, &(this->_status), (void *)this);
        this->_call_state = CallState::WAIT_FINISH;
        LOG(info) << "RequestRobotInfo[" << this->_id << "] Wait for response...";
    }

  ~OnQueryRobotInfo() {
    // LOG(info) << "OnQueryRobotInfo session deleted."; // Do not output log
    // here. It might be after LOG system has been destroyed.
  }
  bool OnCompletionEvent(bool ok) override {
    if (ok) {
      if (this->_call_state == CallState::WAIT_FINISH) {
          LOG(info) << "RequestRobotInfo[" << this->_id << "] Get response.";
          {
              std::lock_guard<std::mutex> lock(this->_proc_mtx);

              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\tname : " << _response.name();
              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\tversion : " << _response.version();
              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\tauthor : " << _response.author();
              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\tdescription : " << _response.description();
              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\tserial(id) : " << _response.serial() << "(" << _response.id() << ")";
              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\ttype : " << _response.type();
              LOG(trace) << "RequestRobotInfo[" << this->_id << "]\tdof : " << _response.dof();

              _call_state = CallState::FINISHED;
          }
        return false; // remove this call
      } else {
          LOG(err) << "RequestRobotInfo[" << this->_id << "] Invalid call state (" << static_cast<int>(_call_state) << ")";
          ABSL_ASSERT(false && "Invalid Call State.");
          return false;
      }
    }
    return false; // remove this call
  }

private:
  ::google::protobuf::Empty _request;
  ::dtproto::robot_msgs::RobotInfo _response;
  std::unique_ptr<::grpc::ClientAsyncResponseReader<::dtproto::robot_msgs::RobotInfo>> _responder;
  int _call_id{-1};
};

class RpcClient : public dt::DAQ::ServiceCallerGrpc<ServiceType>
{
public:
    RpcClient(const std::string &server_address)
        : dt::DAQ::ServiceCallerGrpc<ServiceType>(server_address) {}
};

/////////////////////////////////////////////////////////////////////////
// main
//
int main(int argc, char **argv) {
    dt::Log::Initialize("grpc_service_caller"); //, "logs/grpc_service_caller.txt");
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);

    // initialize RPC client
    std::unique_ptr<RpcClient> rpcClient =
        std::make_unique<RpcClient>("localhost:50052");

    for (int cid = 0; cid < 5; cid++)
    {
        rpcClient->template StartCall<OnQueryRobotInfo>(&cid);
    }

  std::atomic<bool> bRun{true};
  while (bRun.load()) {
    std::cout << "(type \'q\' to quit) >\n";
    std::string cmd;
    std::cin >> cmd;
    if (cmd == "q" || cmd == "quit") {
      bRun = false;
    }
  }
  rpcClient->Stop();

  dt::Log::Terminate(); // flush all log messages
  return 0;
}
