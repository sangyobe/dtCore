// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_TASK_TASKMANAGERSINGLEQUEUE_H__
#define __DT_TASK_TASKMANAGERSINGLEQUEUE_H__

/** \defgroup dtTask
 *
 */

#include "dtTaskManager.h"

namespace dt
{
namespace Task
{

/**
 * A task manager that has a single queue and executes requested tasks sequentially one by one.
 */
class TaskManagerSingleQueue : public TaskManager
{
public:
    TaskManagerSingleQueue()
    {
    }
    virtual ~TaskManagerSingleQueue()
    {
    }

public:
    void UpdateTasks() override
    {
        std::lock_guard<std::mutex> lock(_taskQueueMtx);
        for (auto it : _taskMap)
        {
            it.second->Update();
        }
    }
};

} // namespace Task
} // namespace dt

#endif // __DT_TASK_TASKMANAGERSINGLEQUEUE_H__