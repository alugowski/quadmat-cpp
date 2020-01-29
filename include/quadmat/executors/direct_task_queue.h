// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INCLUDE_QUADMAT_EXECUTORS_DIRECT_TASK_QUEUE_H_
#define QUADMAT_INCLUDE_QUADMAT_EXECUTORS_DIRECT_TASK_QUEUE_H_

#include <queue>

#include "quadmat/executors/task.h"

namespace quadmat {

    /**
     * A comparator for tasks based on their priority.
     */
    struct TaskPriorityComparator {
        constexpr bool operator()(const std::shared_ptr<Task>& lhs, const std::shared_ptr<Task>& rhs) const
        {
            return lhs->GetPriority() < rhs->GetPriority();
        }
    };

    /**
     * A naive task queue that simply executes what it gets. Not thread safe.
     *
     * @tparam Config defines the allocator used for the queue
     */
    template <typename Config>
    class DirectTaskQueue : public TaskQueue {
    public:
        void Enqueue(std::shared_ptr<Task> task) override {
            if (is_executing_) {
                task_queue_.push(task);
            } else {
                is_executing_ = true;
                task->Execute();
                ExecuteAll();
                is_executing_ = false;
            }
        }

    protected:
        void ExecuteAll() {
            while (!task_queue_.empty()) {
                task_queue_.top()->Execute();
                task_queue_.pop();
            }
        }

        /**
         * Queue of tasks to execute. Tasks can enqueue more tasks.
         */
        std::priority_queue<std::shared_ptr<Task>, std::vector<std::shared_ptr<Task>, typename Config::template TempAllocator<std::shared_ptr<Task>>>, TaskPriorityComparator> task_queue_;

        /**
         * Necessary to correctly handle tasks enqueued by other tasks. If a task enqueues another task then the
         * new task will only be run after the first task finishes.
         */
        bool is_executing_ = false;
    };
}

#endif //QUADMAT_INCLUDE_QUADMAT_EXECUTORS_DIRECT_TASK_QUEUE_H_
