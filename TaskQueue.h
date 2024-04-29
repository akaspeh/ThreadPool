//
// Created by PC on 20.04.2024.
//

#ifndef LAB2_TASKQUEUE_H
#define LAB2_TASKQUEUE_H
#include <queue>
#include <iostream>
#include <thread>
#include <shared_mutex>
#include <mutex>
#include "tracy/Tracy.hpp"

using read_write_lock = std::shared_mutex;
using read_lock = std::shared_lock<read_write_lock>;
using write_lock = std::unique_lock<read_write_lock>;


template <typename task_type_t>
class TaskQueue {
    using task_queue_implementation = std::queue<task_type_t>;
public:
    TaskQueue() = default;
    ~TaskQueue() { clear(); }
    bool empty() const;
    size_t size() const;
public:
    void clear();
    bool pop(task_type_t& task);
    template <typename... arguments>
    void emplace(arguments&&... parameters);
public:
    TaskQueue(const TaskQueue& other) = delete;
    TaskQueue(TaskQueue&& other) = delete;
    TaskQueue& operator=(const TaskQueue& rhs) = delete;
    TaskQueue& operator=(TaskQueue&& rhs) = delete;
private:
    mutable read_write_lock m_rw_lock;
    task_queue_implementation m_tasks;
};

template <typename task_type_t>
bool TaskQueue<task_type_t>::empty() const
{
    read_lock _(m_rw_lock);
    return m_tasks.empty();
}
template <typename task_type_t>
size_t TaskQueue<task_type_t>::size() const
{
    read_lock _(m_rw_lock);
    return m_tasks.size();
}

template <typename task_type_t>
void TaskQueue<task_type_t>::clear()
{
    write_lock _(m_rw_lock);
    while (!m_tasks.empty())
    {
        m_tasks.pop();
    }
}
template <typename task_type_t>
bool TaskQueue<task_type_t>::pop(task_type_t& task)
{
    write_lock _(m_rw_lock);
    if (m_tasks.empty())
    {
        return false;
    }
    else
    {
        task = std::move(m_tasks.front());
        m_tasks.pop();
        return true;
    }
}

template <typename task_type_t>
template <typename... arguments>
void TaskQueue<task_type_t>::emplace(arguments&&... parameters)
{
    write_lock _(m_rw_lock);
    m_tasks.emplace(std::forward<arguments>(parameters)...);
}


#endif //LAB2_TASKQUEUE_H
