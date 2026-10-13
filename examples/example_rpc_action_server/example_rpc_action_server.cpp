#include <iostream>
#include <string>
#include <sstream>
#include <signal.h>
#include <dtCore/dtLog>
#include <dtCore/dtThread>
#include <dtCore/src/dtRpc/dtRpcServer.h>
#include <dtCore/src/dtRpc/dtRpcActionServer.h>
#include <dtCore/src/dtRpc/dtRpcUtil.h>
#include <rlab/ponytal/behavior_ctrl_service.grpc.pb.h>

class SetBehaviorActionServer : public dt::Rpc::ActionServer<rlab::ponytal::SetBehaviorReq, rlab::ponytal::SetBehaviorRsp>
{
    using RequestType = rlab::ponytal::SetBehaviorReq;
    using ResponseType = rlab::ponytal::SetBehaviorRsp;

public:
    SetBehaviorActionServer(
        dt::Rpc::Server *server,
        grpc::Service *service,
        grpc::CompletionQueue *cq_call,
        grpc::ServerCompletionQueue *cq_ntfy,
        void *udata)
        : ActionServer<RequestType, ResponseType>(server, service, cq_call, cq_ntfy)
        , _udata(udata)
    {
        // prepare async RPC service
        _session_state = SessionState::WAIT_CONNECT;
        LOG(debug) << "NEW SetBehaviorActionServer(" << this->_id << ") created.";
        (static_cast<rlab::ponytal::BehaviorCtrlService::AsyncService *>(service))->RequestSetBehavior(&_server_ctx, &_request, &_responder, cq_call, cq_ntfy, &_call_context);
    }

    ~SetBehaviorActionServer()
    {
        LOG(debug) << "SetBehaviorActionServer(" << this->_id << ") destroyed.";
    }

    void OnConnect() override
    {
        LOG(info) << "SetBehaviorActionServer connected: " << dt::Rpc::getReadablePeerAddress(_server_ctx.peer());
        _server->AddSession<SetBehaviorActionServer>(rlab::ponytal::BehaviorCtrlService::service_full_name(), _udata);
    }

    void OnRequest(const RequestType &request) override
    {
        LOG(info) << "SetBehaviorActionServer::OnRequest";
        LOG(info) << "hdr.id:" << request.header().seq();

        if (!request.behavior().empty())
        {
            LOG(info) << "behavior:" << request.behavior();
        }
        else
        {
            // send ack. to RPC client with error
            dt::Rpc::touchTimestamp(_response.mutable_header()->mutable_timestamp());
            _response.mutable_header()->set_seq(0);
            _response.mutable_header()->set_response_code(rlab::common::ResponseHeader_ResponseCode_ERR_INVALID_PARAM); // error
            _response.mutable_header()->set_response_desc("Not enough data.");
            Respond(_response);
        }
    }

    void StreamData() override
    {
        if (_session_state == SessionState::WAIT_CONNECT)
            return;

        LOG(trace) << "SetBehaviorActionServer::StreamData(" << _msgSeq << ")";

        /////////////////////////////////////////////////////////////////////////////////////////////
        // update header
        //
        dt::Rpc::touchTimestamp(_response.mutable_header()->mutable_timestamp());
        _response.mutable_header()->set_seq(_msgSeq++);

        /////////////////////////////////////////////////////////////////////////////////////////////
        // update response message
        //
        if (_msgSeq >= 10)
        {
            _response.mutable_header()->set_response_code(rlab::common::ResponseHeader_ResponseCode_OK);
            _response.set_is_tree_def(false);
            _response.set_behavior_id(0);
            _response.set_error(0);
            _response.set_status(0);

            Respond(_response);
        }
        else
        {
            _response.mutable_header()->set_response_code(rlab::common::ResponseHeader_ResponseCode_RUNNING);
            _response.set_is_tree_def(false);
            _response.set_behavior_id(0);
            _response.set_error(0);
            _response.set_status(0);

            Feedback(_response);
        }
    }

private:
    void *_udata{nullptr};
    uint32_t _msgSeq{0};
};

std::unique_ptr<dt::Rpc::Server> server{nullptr};
std::atomic<bool> bRun{true};

void CatchSignal(int sig)
{
    LOG(info) << "catch signal: " << sig;
    bRun = false;
}

int main(int argc, char *argv[])
{
    signal(SIGTERM, CatchSignal); // kill command
    signal(SIGINT, CatchSignal);  // keyboard interrupt, Ctrl + c

    dt::Log::Initialize(argv[0]);
    dt::Log::SetLogLevel(dt::Log::LogLevel::trace);
    LOG(info) << "dtRpc test client.";

    try
    {
        std::string address = "0.0.0.0:50031";

        std::string serviceName = rlab::ponytal::BehaviorCtrlService::service_full_name();
        std::vector<std::pair<std::string, std::unique_ptr<::grpc::Service>>> services;
        services.emplace_back(serviceName, std::make_unique<rlab::ponytal::BehaviorCtrlService::AsyncService>());
        server = std::make_unique<dt::Rpc::Server>(services, address);
        server->AddSession<SetBehaviorActionServer>(serviceName, nullptr);
        // dt::Thread::SleepForMillis(1000);

        while (bRun)
        {
            server->UpdateSessions();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOG(info) << "> stopping...";
        server->Stop();
        server.reset();
        LOG(info) << "> stopped.";
        LOG(info) << "> Press any key and [Enter] to exit...";
    }
    catch(const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
    }

    LOG(info) << "Program terminated.";
    dt::Log::Terminate();
    
    return 0;
}