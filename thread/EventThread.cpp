#include "EventThread.h"

namespace ExNs
{
    event_thread::event_thread()
        : is_running_(false),
          is_notified_(false)
    {}
 
    event_thread::~event_thread()
    {
        stop();
    }
 
    event_thread::event_thread(event_thread&& pt)
    {
        swap(std::move(pt));
    }
 
    event_thread& event_thread::operator=(event_thread&& pt)
    {
        if (this != &pt)
        {
            stop();
 
            swap(std::move(pt));
        }
        return *this;
    }
 
    void event_thread::start()
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        worker_thread_ = std::thread(&event_thread::do_work, this);
        is_running_ = true;
    }
 
    void event_thread::stop()
    {
        {
            std::lock_guard<std::mutex> lock(thread_mutex_);
            is_running_ = false;
            is_notified_ = true;
        }
        thread_cond_.notify_one();
        if (worker_thread_.joinable())
        {
            worker_thread_.join();
        }
    }
 
    void event_thread::swap(event_thread&& pt)
    {
        std::unique_lock<std::mutex> lock_a(thread_mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_b(pt.thread_mutex_, std::defer_lock);
        std::lock(lock_a, lock_b);
 
        std::swap(fct_, pt.fct_);
        std::swap(queue_, pt.queue_);
        std::swap(is_notified_, pt.is_notified_);
    }
 
    void event_thread::do_work()
    {
        bool stop = false;
        std::unique_lock<std::mutex> lock(thread_mutex_);
        while (is_running_ && !stop)
        {
            // boucle pour empêcher les réveils intempestifs
            while (!is_notified_)
            {
                thread_cond_.wait(lock);
            }
            is_notified_ = false;
            while (is_running_ && !stop && !queue_.empty())
            {
                if (fct_)
                {
                    // Reception
                    lock.unlock();
                    stop = fct_();
                    lock.lock();
                }
                else
                {
                    stop = true;
                }
                queue_.pop();
            }
        }
    }
}
