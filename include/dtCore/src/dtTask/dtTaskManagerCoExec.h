// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_TASK_TASKMANAGERCOEXEC_H__
#define __DT_TASK_TASKMANAGERCOEXEC_H__

/** \defgroup dtTask
 *
 */

#include "dtTaskManager.h"

namespace dt
{
namespace Task
{

/**
 * A task manager where all requested tasks are executed simultaneously.
 */
class TaskManagerCoExec : public TaskManager
{
public:
    TaskManagerCoExec()
    {
    }
    virtual ~TaskManagerCoExec()
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

#endif // __DT_TASK_TASKMANAGERCOEXEC_H__