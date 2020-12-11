#pragma once

#include <mutex>
#include <condition_variable>
#include <set>
#include <vector>
#include <algorithm>

namespace common {

template<typename EventType>
class EventMngr
{
public:
    void notify(EventType e)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        events_.insert(e);
        cv_.notify_all();
    }

    void wait()
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&]{return !events_.empty();});
    }

    void wait(EventType e)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&]{return (events_.find(e) != events_.end());});
    }

    void wait_any(std::vector<EventType> events)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&]
            {
                return std::any_of(events.cbegin(), events.cend(),
                                   [&](EventType e){return (events_.find(e) != events_.end());});
            });
    }

    void wait_all(std::vector<EventType> events)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        cv_.wait(lk, [&]
            {
                return std::all_of(events.cbegin(), events.cend(),
                                   [&](EventType e){return (events_.find(e) != events_.end());});
            });
    }

    std::cv_status wait_for(EventType e, std::chrono::milliseconds timeout)
    {
        std::unique_lock<std::mutex> lk(mutex_);
        return cv_.wait(lk, timeout, [&]{return (events_.find(e) != events_.end());});
    }

    bool erase(EventType e)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        return events_.erase(e);
    }

    bool contains(EventType e)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        return !(events_.find(e) == events_.end());
    }

    void clear()
    {
        std::lock_guard<std::mutex> lk(mutex_);
        events_.clear();
    }

private:
    std::set<EventType>     events_;
    std::mutex              mutex_;
    std::condition_variable cv_;
};

} /* namespace common */

