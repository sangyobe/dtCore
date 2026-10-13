// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_ACTIONCLIENT_H__
#define __DT_RPC_ACTIONCLIENT_H__

/** \defgroup dtRpc
 *
 */

#include <dtCore/dtLog>
#include "dtRpcClientSession.h"

namespace dt
{
namespace Rpc
{

template <typename RequestType, typename ResponseType>
class ActionClient : public ClientSession
{
public:
    ActionClient(
        void *stub, 
        grpc::CompletionQueue *cq)
        : ClientSession(stub, cq)
    {
    }
    virtual ~ActionClient() = default;

    virtual bool OnCompletionEvent(bool ok, void *tag) override
    {
        // std::lock_guard<std::mutex> lock(_proc_mtx);

        if (ok)
        {
            if (_session_state == SessionState::WAIT_CONNECT)
            {
                LOG(debug) << "ActionClient::OnCompletionEvent@WAIT_CONNECT";
                _response_reader->Read(&_response, &_call_context);
                _session_state = SessionState::WAIT_READ_DONE;
                return true;
            }
            else if (_session_state == SessionState::WAIT_READ_DONE)
            {
                LOG(debug) << "ActionClient::OnCompletionEvent@WAIT_READ_DONE";
                OnFeedback(_response);
                _session_state = SessionState::WAIT_READ_DONE;
                _response_reader->Read(&_response, &_call_context);
                return true;
            
            }
            else if (_session_state == SessionState::WAIT_FINISH)
            {
                LOG(debug) << "ActionClient::OnCompletionEvent@WAIT_FINISH";
                if (_call_status.ok())
                {
                    LOG(debug) << "ActionClient Session ends successfully.";
                }
                else
                {
                    LOG(err) << "ActionClient Session ends with error code(" << _call_status.error_code() << "): " 
                             << _call_status.error_message();
                }
                _session_state = SessionState::FINISHED;
                return false;
            }
        }
        else // server ends
        {
            if (_session_state == SessionState::WAIT_READ_DONE)
            {
                LOG(debug) << "ActionClient Server-stream ends.";
                LOG(debug) << "ActionClient::OnCompletionEvent@WAIT_READ_DONE";
                OnResponse(_response);
                _session_state = SessionState::WAIT_FINISH;
                _response_reader->Finish(&_call_status, &_call_context);
                return true;
            }
        }
        return false;
    }

    virtual void OnFeedback(const ResponseType &feedback) = 0;
    virtual void OnResponse(const ResponseType &response) = 0;

    SessionState GetSessionState() { return _session_state; }
    ResponseType GetResponse() { return _response; }

protected:
    RequestType _request;
    ResponseType _response;
    std::unique_ptr<grpc::ClientAsyncReader<ResponseType>> _response_reader{nullptr};
    CallContext _call_context{this};
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_ACTIONCLIENT_H__