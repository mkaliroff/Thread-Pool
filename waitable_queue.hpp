#ifndef WAITABLE_QUEUE_HPP
#define WAITABLE_QUEUE_HPP

#include <queue>                // std::queue
#include <condition_variable>   // std::condition_variable
#include <mutex>                // std::mutex
#include <chrono>               // std::nanoseconds
/******************************************************************************/
template <typename T, class CONTAINER = std::queue<T>>
class WaitableQueue
{
public:
    WaitableQueue() = default;
    ~WaitableQueue() = default;
    WaitableQueue(const WaitableQueue &rhs) = delete;
    WaitableQueue &operator=(const WaitableQueue &rhs) = delete;
    
    void Push(const T &data);
    void Pop(T &outparam);
    bool Pop(T &outparam, std::chrono::nanoseconds timeout);
    bool IsEmpty(void) const;

private:
    CONTAINER m_container_queue;
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cond_var;
};
/******************************************************************************/
template <typename T, class CONTAINER>
void WaitableQueue<T, CONTAINER>::Push(const T &data)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    {
        m_container_queue.push(data);
    }
    m_cond_var.notify_one();    
}
/******************************************************************************/
template <typename T, class CONTAINER>
void WaitableQueue<T, CONTAINER>::Pop(T &outparam)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cond_var.wait(lock, []{return false == m_container_queue.empty()});
    outparam = m_container_queue.front();
    m_container_queue.pop();
}
/******************************************************************************/
template <typename T, class CONTAINER>
bool WaitableQueue<T, CONTAINER>::Pop(T &outparam, std::chrono::nanoseconds timeout)
{
    std::chrono::time_point<std::chrono::system_clock> end_timeout = \
                        std::chrono::system_clock::now() + timeout;

    std::unique_lock<std::mutex> lock(m_mutex);
    while(m_container_queue.empty())
    {
        if (std::cv_status::timeout == m_cond_var.wait_until(lock, end_timeout))
        {
            return false;
        }
    }
    outparam = m_container_queue.front();
    m_container_queue.pop();
    return true;
}
/******************************************************************************/
template <typename T, class CONTAINER>
bool WaitableQueue<T, CONTAINER>::IsEmpty(void) const
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_container_queue.empty();
}
/******************************************************************************/

#endif // WAITABLE_QUEUE_HPP