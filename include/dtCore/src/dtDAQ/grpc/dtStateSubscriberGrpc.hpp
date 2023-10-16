// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DTCORE_DTSTATESUBSCRIBERGRPC_H__
#define __DTCORE_DTSTATESUBSCRIBERGRPC_H__

/** \defgroup dtDAQ
 *
 */

#include <grpc/grpc.h>
#include <grpcpp/channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "dtProto/Service.grpc.pb.h"

namespace dtCore {

template<typename StateType>
class dtStateSubscriberGrpc {
public:
    dtStateSubscriberGrpc(const std::string& topic_name, const std::string& server_address);
    ~dtStateSubscriberGrpc();

    void RegMessageHandler(std::function<void(StateType&)>);

private:
    bool InitRequest();
    bool OnCompletionEvent();
    bool TryCancelCallAndShutdown();
    void Stop();

private:
    std::unique_ptr<dtproto::service::DataService::Stub> _stub {nullptr};
    grpc::ClientContext _context;
    grpc::Status _status;
    enum class RpcCallStatus {
        WAIT_START,
        READY_TO_READ,
        WAIT_READ_DONE,
        WAIT_FINISH,
        FINISHED,
        PEER_DISCONNECTED
    };
    RpcCallStatus _call_status {RpcCallStatus::WAIT_START};
    grpc::CompletionQueue _cq;
    std::atomic<bool> _running {true};
    std::thread _rpc_recv_thread;
    std::mutex _call_mtx;
    std::function<void(StateType&)> _msg_handler {nullptr};
    dtproto::std_msgs::State _msg;
    std::unique_ptr<grpc::ClientAsyncReader<dtproto::std_msgs::State> > _stream_reader;
    std::string _topic_name{""};
    std::string _server_address{""};
};

template<typename StateType>
dtStateSubscriberGrpc<StateType>::dtStateSubscriberGrpc(const std::string& topic_name, const std::string& server_address)
: _topic_name(topic_name), _server_address(server_address)
{
    _stub = dtproto::service::DataService::NewStub(
        grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials())
    );

    InitRequest();
}

template<typename StateType>
dtStateSubscriberGrpc<StateType>::~dtStateSubscriberGrpc()
{

}

template<typename StateType>
void dtStateSubscriberGrpc<StateType>::RegMessageHandler(std::function<void(StateType&)> handler)
{
    _msg_handler = handler;
}

template<typename StateType>
bool dtStateSubscriberGrpc<StateType>::InitRequest() {

    dtproto::std_msgs::Request req;
    req.set_name(_topic_name);
    req.set_type(dtproto::std_msgs::Request::ON);
    _stream_reader =
      _stub->PrepareAsyncStreamState(
        &_context,
        req,
        &_cq);
    _stream_reader->StartCall((void*)this);
    _call_status = RpcCallStatus::WAIT_START;


    _running = true;

    _rpc_recv_thread = std::thread([this] {
        // LOG(INFO) << "RPC new-call handler()";
        void* tag;
        bool ok;
        while (_running.load() && _cq.Next(&tag, &ok)) {
            // LOG(INFO) << "Async READ complete event";
            if (ok) {
                static_cast<dtStateSubscriberGrpc*>(tag)->OnCompletionEvent();
            }
            else {
                static_cast<dtStateSubscriberGrpc*>(tag)->TryCancelCallAndShutdown();
            }
        }
    });

    return true;
}

template<typename StateType>
bool dtStateSubscriberGrpc<StateType>::OnCompletionEvent() {

    if (_call_status == RpcCallStatus::WAIT_START) {

        if (!_status.ok()) {
            std::cout << "StreamState rpc call failed." << std::endl;
            return false;
        }

        _stream_reader->Read(&_msg, (void*)this);
        _call_status = RpcCallStatus::WAIT_READ_DONE;

    }
    else if (_call_status == RpcCallStatus::WAIT_READ_DONE) {

        if (!_status.ok()) {
            std::cout << "StreamState rpc stream broken." << std::endl;
            return false;
        }

        if (_msg_handler) {
            StateType state;
            _msg.state().UnpackTo(&state);
            _msg_handler(state);
        }

        _stream_reader->Read(&_msg, (void*)this);
        _call_status = RpcCallStatus::WAIT_READ_DONE;

    }
    else if (_call_status == RpcCallStatus::WAIT_FINISH) {
        _call_status = RpcCallStatus::FINISHED;
        _running = false;
        // Once we're complete, deallocate the call object.
        delete this;
    }
    else {
        // LOG(INFO) << "AsyncStreamStateRequest::OnCompletionEvent, _call_status = " << static_cast<int>(_call_status);
        return false;
    }

    return true;
}

template<typename StateType>
bool dtStateSubscriberGrpc<StateType>::TryCancelCallAndShutdown() {
    _call_status = RpcCallStatus::WAIT_FINISH;
    _stream_reader->Finish(&_status, (void *)this);
    _running = false;
    return true;
}

template<typename StateType>
void dtStateSubscriberGrpc<StateType>::Stop()
{
    TryCancelCallAndShutdown();
}

} // namespace dtCore

#endif // __DTCORE_DTSTATESUBSCRIBERGRPC_H__