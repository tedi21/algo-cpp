#include "PeriodicThread.h"
  
namespace ExNs
{
    periodic_thread::periodic_thread()
    : is_running_(false),
      is_notified_(false)
    {}
     
    periodic_thread::~periodic_thread()
    {
        stop();
    }
     
    periodic_thread::periodic_thread(periodic_thread&& pt)
    {
        swap(std::move(pt));
    }
     
    periodic_thread& periodic_thread::operator=(periodic_thread&& pt)
    {
        if (this != &pt)
        {
            stop();
            swap(std::move(pt));
        }
        return *this;
    }
     
    void periodic_thread::start()
    {
        std::lock_guard<std::mutex> lock(thread_mutex_);
        t_ = std::chrono::steady_clock::now();
        worker_thread_ = std::thread(&periodic_thread::do_work, this);
        is_running_ = true;
    }
     
    void periodic_thread::stop()
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
     
    void periodic_thread::swap(periodic_thread&& pt)
    {
        std::unique_lock<std::mutex> lock_a(thread_mutex_, std::defer_lock);
        std::unique_lock<std::mutex> lock_b(pt.thread_mutex_, std::defer_lock);
        std::lock(lock_a, lock_b);
         
        std::swap(fct_, pt.fct_);
        std::swap(periode_, pt.periode_);
        std::swap(is_notified_, pt.is_notified_);
    }
     
    void periodic_thread::do_work()
    {
        bool stop = false;
        std::unique_lock<std::mutex> lock(thread_mutex_);
        while (is_running_ && !stop)
        {
            // temps suivant
            t_ = t_ + periode_;
             
            // boucle pour empêcher les rêveils intempestifs
            std::cv_status timeOut = std::cv_status::no_timeout;
            while ((!is_notified_) && (timeOut == std::cv_status::no_timeout))
            {
                // Calcul du temps
                std::chrono::steady_clock::time_point t2 =
                std::chrono::steady_clock::now();
                std::chrono::milliseconds next =
                    std::chrono::duration_cast
                        <std::chrono::milliseconds>(t_ - t2);
                if (next < std::chrono::milliseconds::zero())
                {
                    // Retard de plus d'un cycle
                    next = periode_;
                    t_ = t2 + next;
                }
                else if (next > periode_)
                {
                    // En avance
                    next = periode_;
                    t_ = t2 + next;
                }
                timeOut = thread_cond_.wait_for(lock, next);
            }
            is_notified_ = false;
             
            if (is_running_ && fct_)
            {
                // Tick
                lock.unlock();
                stop = fct_();
                lock.lock();
            }
            else
            {
                stop = true;
            }
        }
    }
}
