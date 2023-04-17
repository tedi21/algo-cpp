#ifndef PERIODIC_THREAD_H
#define PERIODIC_THREAD_H
#include <memory>
#include <condition_variable>
#include <chrono>
#include <mutex>
#include <thread>
#include <functional>
  
namespace ExNs
{
    class periodic_thread
    {
    public:
        periodic_thread();
         
        template<typename Callable, typename... Args>
        explicit periodic_thread(const std::chrono::milliseconds& periode,
        Callable&& fct, Args&&... args)
        : periode_(periode),
          is_running_(false),
          is_notified_(false)
        {
            fct_ = std::bind(std::forward<Callable>(fct),
            std::forward<Args>(args)...);
        }
         
        ~periodic_thread();
         
        periodic_thread(const periodic_thread&) = delete;
        periodic_thread& operator=(const periodic_thread&) = delete;
         
        periodic_thread(periodic_thread&&);
        periodic_thread& operator=(periodic_thread&&);
         
        void start();
         
    private:
        void swap(periodic_thread&&);
        void stop();
        void do_work();
         
        std::function<bool()> fct_;
        std::chrono::milliseconds periode_;
        std::chrono::steady_clock::time_point t_;
        bool is_running_;
        bool is_notified_;
        std::condition_variable thread_cond_;
        std::mutex thread_mutex_;
        std::thread worker_thread_;
    };
}
  
#endif // PERIODIC_THREAD_H