#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <dtProto/Service.grpc.pb.h>

#include <memory>
#include <string>

/////////////////////////////////////////////////////////////////////////
// RpcClient (Rpc service caller)
//
class RpcClient {
public:
    RpcClient(const std::string& server_address);

    bool RequestRobotInfo();
    bool RequestVersion();
    bool ControlCmd(int cmd_mode, const char* fmt, ...);

private:
    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<dtproto::dtService::Stub> stub_;
};

RpcClient::RpcClient(const std::string& server_address)
    : channel_(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()))
    , stub_(dtproto::dtService::NewStub(channel_)) 
{
}

bool RpcClient::RequestRobotInfo()
{
    grpc::ClientContext context;
    ::google::protobuf::Empty req;
    dtproto::robot_msgs::RobotInfo res;

    grpc::Status status = stub_->RequestRobotInfo(&context, req, &res);
    if (!status.ok()) {
        std::cout << "RequestRobotInfo rpc failed." << std::endl;
        return false;
    } else {
        std::cout << "name : " << res.name() << std::endl;
        std::cout << "version : " << res.version() << std::endl;
        std::cout << "author : " << res.author() << std::endl;
        std::cout << "description : " << res.description() << std::endl;
        std::cout << "serial(id) : " << res.serial() << "(" << res.id() << ")" << std::endl;
        std::cout << "type : " << res.type() << std::endl;
        std::cout << "dof : " << res.dof() << std::endl;
        return true;
    }
}

bool RpcClient::RequestVersion()
{
    grpc::ClientContext context;
    ::google::protobuf::Empty req;
    ::google::protobuf::StringValue res;

    grpc::Status status = stub_->RequestVersion(&context, req, &res);
    if (!status.ok())
    {
        std::cout << "RequestVersion rpc failed." << std::endl;
        return false;
    }
    else
    {
        std::cout << "version : " << res.value() << std::endl;
        return true;
    }
}

bool RpcClient::ControlCmd(int cmd_mode, const char* fmt, ...) 
{
    grpc::ClientContext context;
    dtproto::robot_msgs::ControlCmd req;
    dtproto::std_msgs::Response res;
    
    req.set_cmd_mode(cmd_mode);
    // req.set_arg(arg);

    grpc::Status status = stub_->Command(&context, req, &res);
    if (!status.ok()) {
        std::cout << "Command rpc failed." << std::endl;
        return false;
    } else {
        std::cout << "rtn : " << res.rtn() << std::endl;
        std::cout << "msg : " << res.msg() << std::endl;
        return true;
    }
}

/////////////////////////////////////////////////////////////////////////
// main
//
// int main(int argc, char **argv)
// {
//     // initialize RPC client
//     std::unique_ptr<RpcClient> rpcClient = std::make_unique<RpcClient>("localhost:50052");
//     // std::cout << "> ControlCmd() call through the 1st channel...\n";
//     // rpcClient->ControlCmd(1, "");
//     std::cout << "> RequestRobotInfo() call through the 1st channel...\n";
//     rpcClient->RequestRobotInfo();
//     std::cout << "> RequestVersion() call through the 1st channel...\n";
//     rpcClient->RequestVersion();

//     // open another channel
//     std::cout << "> Open another channel to RPC server...\n";
//     std::unique_ptr<RpcClient> rpcClient_2 = std::make_unique<RpcClient>("localhost:50052");
//     std::cout << "> RequestRobotInfo() call through the 2nd channel...\n";
//     rpcClient_2->RequestRobotInfo();

//     return 0;
// }

int main(int argc, char **argv)
{
    std::string server_address = "localhost:50052";
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    std::unique_ptr<dtproto::dtService::Stub> stub = dtproto::dtService::NewStub(channel);

    grpc::ClientContext context;
    ::google::protobuf::Empty req;
    dtproto::robot_msgs::RobotInfo res;
    grpc::Status status = stub->RequestRobotInfo(&context, req, &res);
    if (!status.ok())
    {
        std::cout << "RequestRobotInfo rpc failed." << std::endl;
    }
    else
    {
        std::cout << "name : " << res.name() << std::endl;
        std::cout << "version : " << res.version() << std::endl;
        std::cout << "author : " << res.author() << std::endl;
        std::cout << "description : " << res.description() << std::endl;
        std::cout << "serial(id) : " << res.serial() << "(" << res.id() << ")" << std::endl;
        std::cout << "type : " << res.type() << std::endl;
        std::cout << "dof : " << res.dof() << std::endl;
    }

    return 0;
}
