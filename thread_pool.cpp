#include <unistd.h> // nice

#include "thread_pool.hpp"
#include "callable.hpp"

/*****************************************************************************/
enum ExtendedPriority {sleeping_pill = ThreadPool::Priority::num_of_priorities + 1, exterminate_thread};
/*****************************************************************************/
ThreadPool::ThreadPool(int niceness, size_t numOfThreads) : \
m_niceness(niceness), m_status_switch_1(false), m_status_switch_2(false), m_num_thread_target(numOfThreads)
{
    Pause();
    for(size_t i = 0; i < m_num_thread_target; i++)
    {
        std::thread worker(&ThreadPool::Worker, this);
        m_thread.insert({worker.get_id(), std::move(worker)});
    }
}
/*****************************************************************************/
ThreadPool::~ThreadPool()
{
    SetNumOfThreads(0);
}
/*****************************************************************************/
void ThreadPool::Run()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);  
        m_status_switch_1 = true;
        m_status_switch_2 = true;
    }
    m_cond_var.notify_all();
}
/*****************************************************************************/
void ThreadPool::Pause()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status_switch_1 = false;
    }
    std::shared_ptr<Callable> sleeping_pill = std::make_shared<CallableFunction>(SleepingPillSwitch1, this);
    for(size_t i = 0; i < m_num_thread_target; i++)
    {
        AddTask(sleeping_pill, Priority(ExtendedPriority::sleeping_pill));
    }
}
/*****************************************************************************/
void ThreadPool::SetNumOfThreads(size_t num)
{
    size_t current_num_threads = m_num_thread_target;
    m_num_thread_target = num;

    if(num < current_num_threads)
    {
        ReduceThreads(current_num_threads);
    }
    else
    {
        IncreaseThreads(current_num_threads);
    }
}
/*****************************************************************************/
void ThreadPool::IncreaseThreads(size_t current_num_threads)
{
    if(false == m_status_switch_1)
    {
        std::shared_ptr<Callable> sleeping_pill = std::make_shared<CallableFunction>(SleepingPillSwitch1, this);
        for(size_t i = current_num_threads; i < m_num_thread_target; i++)
        {
            AddTask(sleeping_pill, Priority(ExtendedPriority::sleeping_pill));
        }
    }
    for(size_t i = current_num_threads; i < m_num_thread_target; i++)
    {
        std::thread worker(&ThreadPool::Worker, this);
        m_thread.insert({worker.get_id(), std::move(worker)});
    }
}
/*****************************************************************************/
void ThreadPool::ReduceThreads(size_t current_num_threads)
{
    std::shared_ptr<Callable> exterminate_thread = std::make_shared<CallableFunction> (ExterminateThread, this);

    for(size_t i = 0 ; i < current_num_threads - m_num_thread_target  ; i++)
    {
        AddTask(exterminate_thread, Priority(ExtendedPriority::exterminate_thread));
    }

    if(false == m_status_switch_1)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);  
            m_status_switch_2 = false;
        }
        std::shared_ptr<Callable> system_sleeping_pill = std::make_shared<CallableFunction>(SleepingPillSwitch2, this);
        for(size_t i = 0; i < m_num_thread_target; i++)
        {
            AddTask(system_sleeping_pill, Priority(ExtendedPriority::sleeping_pill));
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);  
            m_status_switch_1 = true;
        }
    }
    else if(false == m_status_switch_2)
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);  
            m_status_switch_1 = false;
        }        std::shared_ptr<Callable> sleeping_pill = std::make_shared<CallableFunction>(SleepingPillSwitch1, this);
        for(size_t i = 0; i < m_num_thread_target; i++)
        {
            AddTask(sleeping_pill, Priority(ExtendedPriority::sleeping_pill));
        }
        {
            std::lock_guard<std::mutex> lock(m_mutex);  
            m_status_switch_2 = true;
        }
    }
    m_cond_var.notify_all();

    for(size_t i = 0 ; i < current_num_threads - m_num_thread_target  ; i++)
    {
        EraseThread();
    }
}
/*****************************************************************************/
void ThreadPool::EraseThread()
{
    std::thread::id id;
    m_join_queue.Pop(id);
    std::thread temp = std::move(m_thread[id]);
    temp.join();
    m_thread.erase(id);
}
/*****************************************************************************/
void ThreadPool::AddTask(std::shared_ptr<Callable> TaskToDo, Priority priority)
{
    std::pair<std::shared_ptr<Callable>, Priority> inparam = {TaskToDo, priority};
    m_queue.Push(inparam);
}
/*****************************************************************************/
void ThreadPool::Worker()
{
    nice(m_niceness);
    std::pair<std::shared_ptr<Callable>, Priority> outparam;
    while(true)
    {
        m_queue.Pop(outparam);
        try
        {
            (*outparam.first)();
        }
        catch(const std::runtime_error &e)
        {
            return;
        }
    }    
}
/*****************************************************************************/
void ThreadPool::SleepingPillSwitch1(ThreadPool *object)
{
    std::unique_lock<std::mutex> lock(object->m_mutex);
    object->m_cond_var.wait(lock, [&]{return (object->m_status_switch_1);});
}
/*****************************************************************************/
void ThreadPool::SleepingPillSwitch2(ThreadPool *object)
{
    std::unique_lock<std::mutex> lock(object->m_mutex);
    object->m_cond_var.wait(lock, [&]{return (object->m_status_switch_2);});
}
/*****************************************************************************/
void ThreadPool::ExterminateThread(ThreadPool *object)
{
    object->m_join_queue.Push(std::this_thread::get_id());
    throw std::runtime_error("exterminate_exceptions");
}
/*****************************************************************************/