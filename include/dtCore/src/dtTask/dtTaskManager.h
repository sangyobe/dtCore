// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_TASK_TASKMANAGER_H__
#define __DT_TASK_TASKMANAGER_H__

/** \defgroup dtTask
 *
 */

#include <string>
#include <memory>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include "dtTask.h"

namespace dt
{
namespace Task
{

class Task;

class TaskManager
{
public:
    TaskManager()
    {
    }
    virtual ~TaskManager()
    {
    }

public:
    std::shared_ptr<Task> AddTask(std::shared_ptr<Task> task)
    {
        std::lock_guard<std::mutex> lock(_taskQueueMtx);
        _taskMap.emplace(task->taskId, task);
        return task;
    }

    void RemoveTask(TaskId taskId)
    {
        std::lock_guard<std::mutex> lock(_taskQueueMtx);
        _taskMap.erase(taskId);
    }

    std::shared_ptr<Task> GetTask(TaskId taskId)
    {
        std::lock_guard<std::mutex> lock(_taskQueueMtx);
        auto it = _taskMap.find(taskId);
        if (it != _taskMap.end())
        {
            return it->second;
        }
        return nullptr;
    }

    virtual void UpdateTasks() = 0;

protected:
    std::unordered_map<TaskId, std::shared_ptr<Task>> _taskMap;
    std::mutex _taskQueueMtx;

private: // deleted default copy&assignment constructors
    TaskManager(const TaskManager &) = delete;
    TaskManager(TaskManager &&) = delete;
    TaskManager &operator=(const TaskManager &) = delete;
    TaskManager &operator=(TaskManager &&) = delete;
};

} // namespace Task
} // namespace dt

#endif // __DT_TASK_TASKMANAGER_H__