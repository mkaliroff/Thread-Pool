#ifndef TREAD_POOL_HPP
#define TREAD_POOL_HPP

#include <thread>//hardware_concurrency
#include <memory>//shared_ptr
#include <unordered_map> // unordered map

#include "pq_to_q.hpp"
#include "callable.hpp"//Callable
#include "waitable_queue.hpp"//WaitableQueue
#include "function.hpp" //Function

using container = std::pair<std::shared_ptr<Callable>, Priority>;

class ThreadPool
{
public:
    enum Priority{lowest = 0, low, medium, high, highest, num_of_priorities};

    ThreadPool(int niceness = 0, size_t numOfThreads = std::thread::hardware_concurrency());
    ~ThreadPool();
    ThreadPool(const ThreadPool &rhs) = delete;
    ThreadPool &operator=(const ThreadPool &rhs) = delete;

    void Run();
    void Pause();
    void SetNumOfThreads(size_t num);
    void AddTask(std::shared_ptr<Callable> TaskToDo, Priority priority = medium);
private:
    struct Compare
    {
        bool operator()(container a, container b)
        {
            return a.second < b.second;
        }
    };


    int m_niceness;
    bool m_status_switch_1;
    bool m_status_switch_2;
    size_t m_num_thread_target;
    std::unordered_map<std::thread::id, std::thread> m_thread;
    WaitableQueue <container, FrontablePQ<container, std::vector<container>,Compare>> m_queue;
    WaitableQueue<std::thread::id >m_join_queue;
    std::mutex m_mutex;
    std::condition_variable m_cond_var;

    void Worker();
    void ReduceThreads(size_t current_num_threads);
    void IncreaseThreads(size_t current_num_threads);
    void EraseThread();
    static void SleepingPillSwitch1(ThreadPool *object);
    static void SleepingPillSwitch2(ThreadPool *object);
    static void ExterminateThread(ThreadPool *object);
};

#endif // TREAD_POOL_HPP