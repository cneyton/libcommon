#ifndef WAIT_QUEUE_H
#define WAIT_QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace common
{

template <typename T>
class WaitQueue
{
public:

    T pop()
    {
        std::unique_lock<std::mutex> lk(mutex_);

        cond_.wait(lk, [&] {return !queue_.empty();});

        auto elt = queue_.front();
        queue_.pop();
        return elt;
    }

    void pop(T& elt)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        cond_.wait(lk, [&] {return !queue_.empty();});

        elt = queue_.front();
        queue_.pop();
    }

    void push(const T& elt)
    {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            queue_.push(elt);
        }
        cond_.notify_all();
    }

    void push(T&& elt)
    {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            queue_.push(std::move(elt));
        }
        cond_.notify_all();
    }

    size_t size()  {return queue_.size();}
    bool   empty() {return queue_.empty();}

private:
    std::queue<T>           queue_;
    std::mutex              mutex_;
    std::condition_variable cond_;
};

} /* namespace common */

#endif /* WAIT_QUEUE_H */
