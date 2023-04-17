#ifndef EVENT_THREAD_H
#define EVENT_THREAD_H
#include <memory>
#include <thread>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <functional>
 
namespace ExNs
{
    class event_thread
    {
    public:
        event_thread();
 
        template<typename Callable, typename... Args>
        explicit event_thread(Callable&& fct, Args&&... args)
        : is_running_(false),
          is_notified_(false)
        {
            fct_ = std::bind(std::forward<Callable>(fct), 
                             std::forward<Args>(args)...);
        }
 
        ~event_thread();
 
        event_thread(const event_thread&) = delete;
        event_thread& operator=(const event_thread&) = delete;
 
        event_thread(event_thread&&);
        event_thread& operator=(event_thread&&);
 
        template<typename... Args>
        void notifyEvent(Args... args)
        {
            std::unique_lock<std::mutex> lock(thread_mutex_);
            queue_.push(std::make_shared< event_args<Args...> >(
                            std::forward<Args>(args)...));
            is_notified_ = true;
            lock.unlock();
            thread_cond_.notify_one();
        }
 
        template<typename... Args>
        bool getEvent(std::tuple<Args...>& args)
        {
            std::lock_guard<std::mutex> lock(thread_mutex_);
            std::shared_ptr< event_args<Args...> > ptr
                    = std::dynamic_pointer_cast< event_args<Args...> >(
                          queue_.front());
            if (ptr != nullptr)
            {
                args = ptr->tuple_;
            }
            return (ptr != nullptr);
        }
 
        void start();
 
    private:
        class event_args_base
        {
        public:
            virtual ~event_args_base() = default;
        };
 
        template<typename... Args>
        class event_args final
            : public event_args_base
        {
        public:
            explicit event_args(Args&&... args)
                : tuple_(std::forward<Args>(args)...)
            {}
 
            std::tuple<Args...> tuple_;
        };
 
        void stop();
        void swap(event_thread&&);
        void do_work();
 
        std::function<bool()> fct_;
        std::queue< std::shared_ptr<event_args_base> > queue_;
        bool is_running_;
        bool is_notified_;
        std::condition_variable thread_cond_;
        std::mutex thread_mutex_;
        std::thread worker_thread_;
    };
}
 
#endif // EVENT_THREAD_H