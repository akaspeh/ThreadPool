//
// Created by PC on 24.03.2024.
//
#include "ThreadPool.h"

bool ThreadPool::working() const
{
    read_lock _(m_rw_lock);
    return working_unsafe();
}
bool ThreadPool::working_unsafe() const
{
    return m_initialized && !m_terminated;
}

void ThreadPool::initialize(const size_t worker_count)
{
    ZoneScoped;
    write_lock _(m_rw_lock);
    m_thread_qm = std::thread(&ThreadPool::queue_manager, this);
    m_thread_bm = std::thread(&ThreadPool::buff_manager, this);
    if (m_initialized || m_terminated)
    {
        return;
    }
    m_workers.reserve(worker_count);
    for (size_t id = 0; id < worker_count; id++)
    {
        m_workers.emplace_back(&ThreadPool::routine, this);
    }
    m_initialized = !m_workers.empty();

}

void ThreadPool::routine()
{
    ZoneScoped;
    while (true)
    {
        bool task_accquiered = false;
        std::function<void()> task;
        {
            write_lock _(m_rw_lock);
            if (m_tasks.empty() and m_queue_is_working.load()){
                m_queue_is_working.store(false);
                ZoneScopedN( "QUEUE STOP" );
            }
            auto wait_condition = [this, &task_accquiered, &task] {
                if(!m_queue_is_working.load()){
                    task_accquiered = false;
                }
                else{
                    task_accquiered = m_tasks.pop(task);
                }
                return m_terminated || (task_accquiered && m_queue_is_working.load());
            };
            auto start = std::chrono::steady_clock::now();
            m_task_waiter.wait(_, wait_condition);
            auto end = std::chrono::steady_clock::now();
            auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
            sv.mean_wait_add(milliseconds);
        }
        if (m_terminated && !task_accquiered)
        {
            return;
        }
        task();
    }
}

void ThreadPool::terminate()
{
    ZoneScoped;
    {
        write_lock _(m_rw_lock);
        if (working_unsafe())
        {
            ZoneScopedN( "terminated 1" );
            m_terminated = true;
        }
        else
        {
            ZoneScopedN( "terminated 2" );
            m_workers.clear();
            m_terminated = false;
            m_initialized = false;
            return;
        }
    }
    m_task_waiter.notify_all();
    for (std::thread& worker : m_workers)
    {
        ZoneScopedN( "terminated 3" );
        worker.join();
    }
    m_thread_qm.join();
    m_thread_bm.join();
    m_workers.clear();
    m_terminated = false;
    m_initialized = false;
}

void ThreadPool::queue_manager()
{
    while (true) {
        ZoneScopedN( "queue_manager_push_execute" );
        if (m_terminated)
        {
            return;
        }
        std::this_thread::sleep_for(std::chrono::seconds(40));
        m_queue_is_working.store(true);
        std::string text = std::to_string(m_time_sum);
        ZoneText(text.c_str(), text.size());
        m_time_sum.store(0);
        m_task_waiter.notify_all();
    }
}

void ThreadPool::buff_manager()
{
    while (true) {
        if (m_terminated)
        {
            return;
        }
        while(!m_buff.empty() and !m_queue_is_working.load()){
            auto task_and_time = m_buff.back();
            if(task_and_time.second + m_time_sum.load() > 60){
                break;
            }
            ZoneScopedN( "buff_manager_push_execute" );
            m_buff.pop_back();
            m_time_sum.fetch_add(task_and_time.second);
            m_tasks.emplace(task_and_time.first);
        }
    }
}
