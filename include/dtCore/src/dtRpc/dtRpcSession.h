// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_RPC_SESSION_H__
#define __DT_RPC_SESSION_H__

/** \defgroup dtRpc
 *
 */

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

namespace dt
{
namespace Rpc
{

class Session
{
public:
    Session() : _id(AllocSessionId()) {}
    virtual ~Session() = default;

public:
    /*!
     * Completion event handler.
     * All subclasses should implement their own completion event handler properly.
     * This is called whenever async call requests completed such as Prepare(), Read(), Write(), Finalize(), etc.
     * @param ok
     * @param tag
     * @return true if message is handled successfully. if it returns false, this Call object will be deleted.
     */
    virtual bool OnCompletionEvent(bool ok, void *tag) = 0;

    /*!
     */
    virtual bool TryCancelCallAndShutdown() = 0;

    /*!
     * Update data and send(stream) it to peer.
     */
    virtual void StreamData() = 0;

public:
    /*!
     * Returns id for this Session instance.
     * @return session id.
     */
    uint64_t GetId() { return _id; }

public:
    static uint64_t AllocSessionId()
    {
        static std::atomic<uint64_t> _session_id_allocator{0};
        return (++_session_id_allocator);
    }

protected:
    uint64_t _id;
};

/*!
 * Async RPC function call completion event context.
 */
class CallContext
{
public:
    CallContext(Session* ss, int cs = 0) : session(ss), call_state(cs) {}
    CallContext() = delete;
    Session* session;
    int call_state;
};

} // namespace Rpc
} // namespace dt

#endif // __DT_RPC_SESSION_H__