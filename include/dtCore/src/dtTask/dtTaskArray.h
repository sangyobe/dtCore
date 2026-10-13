// This file is part of dtCore, a C++ library for robotics software
// development.
//
// This library is commercial and cannot be redistributed, and/or modified
// WITHOUT ANY ALLOWANCE OR PERMISSION OF Hyundai Motor Company.

#ifndef __DT_TASK_TASKARRAY_H__
#define __DT_TASK_TASKARRAY_H__

/** \defgroup dtTask
 *
 */

#include <memory>
#include <atomic>
#include <vector>
#include "dtTask.h"

namespace dt
{
namespace Task
{

class TaskArray : public Task
{
public:
    TaskArray()
    {
    }
    virtual ~TaskArray()
    {
    }

    void Update() override
    {
        if (_taskRunning < _taskArray.size())
        {
            _taskArray[_taskRunning]->Update();
            if (_taskArray[_taskRunning]->GetTaskState() == TaskState::FINISHED)
            {
                _taskRunning++;
            }
            else if (_taskArray[_taskRunning]->GetTaskState() == TaskState::CANCELED)
            {
                Finish(_taskArray[_taskRunning]->GetTaskErr());
            }
        }
        else
        {
            Finish(); // TODO: error code 확인
        }
    }

public:
    std::shared_ptr<Task> AddSubTask(std::shared_ptr<Task> task)
    {
        _taskArray.push_back(task);
        return task;
    }

protected:
    std::vector<std::shared_ptr<Task>> _taskArray;
    std::size_t _taskRunning{0}; // the index of the current running task in task list.
};

} // namespace Task
} // namespace dt

#endif // __DT_TASK_TASKARRAY_H__