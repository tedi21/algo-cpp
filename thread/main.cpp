#include <iostream>
#include "EventThread.h"
#include "PeriodicThread.h"

class MyClass
{
private:
    ExNs::event_thread thread_;
public:
    MyClass()
    {
        thread_ = ExNs::event_thread(&MyClass::treat, this);
        thread_.start();
    }
    void notify();
    bool treat();
};

void MyClass::notify()
{
    size_t data = 9999;
    thread_.notifyEvent(data);
}

bool MyClass::treat()
{
    std::tuple<size_t> msg;
    if (thread_.getEvent(msg))
    {
        auto [data] = msg;
        std::cout << "notify " << data << std::endl;
    }
    return false;
}

int main()
{
    ExNs::periodic_thread thread_;
    thread_ = ExNs::periodic_thread(std::chrono::milliseconds(1000), []() {
        std::cout << "tick" << std::endl;
        return false;
    });
    thread_.start();

    MyClass c;
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    c.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    c.notify();
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
}