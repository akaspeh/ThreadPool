//
// Created by PC on 24.03.2024.
//

//Пул потоків обслуговується 4-ма робочими потоками й має одну чергу
//виконання. Задачі додаються відразу в кінець черги виконання. Черга
//задач виконується з інтервалом в 40 секунд (буфер наповнюється
//задачами на протязі 40-ти секунд, котрі потім виконуються), або
//відкидаються, якщо час виконання всіх задач в черзі та потенційної задачі
//для додавання більший ніж 60 секунд. Задачі, що додаються під час
//виконання черги задач відкладаються на наступну ітерацію (подвійна
//буферизація). Задача займає випадковий час від 6 до 14 секунд.

#ifndef LAB2_THREADPOOL_H
#define LAB2_THREADPOOL_H
#include <vector>
#include <functional>
#include "TaskQueue.h"
#include <condition_variable>

class Statistic_Var{
public:
    Statistic_Var() = default;
    ~Statistic_Var() = default;
    std::atomic<long long> mean_wait_thread_time = 0;
    std::atomic<long long> wait_counter = 0;
    void mean_wait_add(long long time){
        wait_counter.fetch_add(1);
        mean_wait_thread_time.fetch_add(time);
    }
    double mean_wait_ms(){
        return (double)mean_wait_thread_time.load() / (double)wait_counter.load();
    }
};

class ThreadPool {
public:
    inline ThreadPool() = default;
    inline ~ThreadPool() { terminate(); }
public:
    void initialize(const size_t worker_count);
    void terminate();
    void routine();
    bool working() const;
    bool working_unsafe() const;
    Statistic_Var sv;
public:
    template <typename task_t, typename... arguments>
    void add_task(task_t&& task, arguments&&... parameters);
    void queue_manager();
    void buff_manager();
public:
    ThreadPool(const ThreadPool& other) = delete;
    ThreadPool(ThreadPool&& other) = delete;
    ThreadPool& operator=(const ThreadPool& rhs) = delete;
    ThreadPool& operator=(ThreadPool&& rhs) = delete;
private:
    mutable read_write_lock m_rw_lock;
    mutable std::condition_variable_any m_task_waiter;
    std::vector<std::function<void()>> buff;
    std::vector<std::thread> m_workers;
    std::thread m_thread_qm;
    std::thread m_thread_bm;
    std::atomic<int> m_time_sum = 0;
    TaskQueue<std::function<void()>> m_tasks;
    std::vector<std::pair<std::function<void()>, int>> m_buff;
    std::atomic<bool> m_queue_is_working = false;
    bool m_initialized = false;
    bool m_terminated = false;
};

template <typename task_t, typename... arguments>
void ThreadPool::add_task(task_t&& task, arguments&&... parameters)
{
    ZoneScoped;
    {
        read_lock _(m_rw_lock);
        if (!working_unsafe()) {
            return;
        }
    }

    if(m_queue_is_working.load()){
        ZoneScopedN( "Buff vec" );
        //тут надо реализовать буфер
        auto bind = std::bind(std::forward<task_t>(task),
                              std::forward<arguments>(parameters)...);
        int int_sum = 0;
        ((int_sum += parameters), ...);
        auto buff_task = std::make_pair(bind, int_sum);
        m_buff.push_back(buff_task);
    }
    else{
        ZoneScopedN( "Main Queue" );
        int int_sum = 0;
        ((int_sum += parameters), ...);
        if(m_time_sum.load() + int_sum > 60){
            ZoneScopedN( "denied" );
            return;
        }
        m_time_sum.fetch_add(int_sum);
        auto bind = std::bind(std::forward<task_t>(task),
                              std::forward<arguments>(parameters)...);
        m_tasks.emplace(bind);
    }
}
#endif //LAB2_THREADPOOL_H
