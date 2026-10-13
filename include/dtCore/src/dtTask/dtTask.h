// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_TASK_TASK_H__
#define __DT_TASK_TASK_H__

/** \defgroup dtTask
 *
 */

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace dt
{
namespace Task
{

typedef uint64_t TaskId;
typedef int32_t TaskErr;

class Task
{
public:
    Task()
        : taskId(++_taskIdAllocator)
    {
    }
    virtual ~Task()
    {
    }

    virtual void OnInitialize() = 0;
    virtual void OnUpdate() = 0;

public:
    TaskId taskId;

    enum class TaskState : int32_t
    {
        WAIT_INITIALIZE,
        INITIALIZED,
        PAUSED,
        RUNNING,
        CANCELED,
        FINISHED
    };
    TaskState taskState{TaskState::WAIT_INITIALIZE};
    void SetTaskState(TaskState newState) // set state value safely across threads using spin-lock
    {
        // bool expected = false;
        // while (!_stateLock.compare_exchange_weak(expected, true));
        taskState = newState;
        // _stateLock = false;
    }
    TaskState GetTaskState() { return taskState; }

    TaskErr taskErr;
    void SetTaskErr(TaskErr err) { taskErr = err; }
    TaskErr GetTaskErr() { return taskErr; }

    virtual void Update()
    {
        switch (taskState)
        {
        case TaskState::WAIT_INITIALIZE:
            OnInitialize();
            break;
        case TaskState::INITIALIZED:
        case TaskState::RUNNING:
        case TaskState::PAUSED:
            OnUpdate();
            break;
        case TaskState::CANCELED:
        case TaskState::FINISHED:
        default:
            break;
        }
    }

    void Initialize()
    {
        SetTaskState(TaskState::INITIALIZED);
    }
    void Run()
    {
        SetTaskState(TaskState::RUNNING);
    }
    void Pause()
    {
        SetTaskState(TaskState::PAUSED);
    }
    void Cancel()
    {
        SetTaskState(TaskState::CANCELED);
        this->taskErr = -1;
    }
    void Finish(TaskErr err = 0)
    {
        this->taskErr = err;
        SetTaskState(TaskState::FINISHED);
    }

private:
    std::atomic<bool> _stateLock{false};

    static inline std::atomic<uint64_t> _taskIdAllocator{0}; // C++17

private: // deleted default copy&assignment constructors
    Task(const Task &) = delete;
    Task(Task &&) = delete;
    Task &operator=(const Task &) = delete;
    Task &operator=(Task &&) = delete;
};

} // namespace Task
} // namespace dt

#endif // __DT_TASK_TASK_H__