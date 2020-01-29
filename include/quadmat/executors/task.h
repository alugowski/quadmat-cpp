// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INCLUDE_QUADMAT_EXECUTORS_TASK_H_
#define QUADMAT_INCLUDE_QUADMAT_EXECUTORS_TASK_H_

namespace quadmat {
    class Task {
    public:
        virtual ~Task() = default;

        virtual void Execute() = 0;

        [[nodiscard]] virtual int GetPriority() const = 0;
    };

    class TaskQueue {
    public:
        virtual ~TaskQueue() = default;

        virtual void Enqueue(std::shared_ptr<Task> task) = 0;
    };
}

#endif //QUADMAT_INCLUDE_QUADMAT_EXECUTORS_TASK_H_
