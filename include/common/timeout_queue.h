/**
 * Simple timeout queue.  Call user-specified callbacks when their timeouts
 * expire.
 *
 * This class assumes that "time" is an int64_t and doesn't care about time
 * units (seconds, milliseconds, etc).  You call run_once() / run_loop() using
 * the same time units that you use to specify callbacks.
 *
 * Adapted from    : https://github.com/facebook/folly
 * Original author : Tudor Bosman (tudorb@fb.com)
 */

#pragma once

#include <cstdint>
#include <functional>
#include <algorithm>
#include <vector>
#include <mutex>

#include <boost/multi_index/indexed_by.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index_container.hpp>

namespace common
{

class TimeoutQueue {
public:
    typedef int64_t Id;
    typedef std::function<void(Id, int64_t)> Callback;

    TimeoutQueue() : nextId_(1) {}

    /**
     * Add a one-time timeout event that will fire "delay" time units from "now"
     * (that is, the first time that run*() is called with a time value >= now
     * + delay).
     */
    Id add(int64_t now, int64_t delay, Callback callback)
    {
        std::unique_lock<std::mutex> lk(mutex_run_);
        Id id = nextId_++;
        timeouts_.insert({id, now + delay, -1, std::move(callback)});
        return id;
    }

    /**
     * Add a repeating timeout event that will fire every "interval" time units
     * (it will first fire when run*() is called with a time value >=
     * now + interval).
     *
     * run*() will always invoke each repeating event at most once, even if
     * more than one "interval" period has passed.
     */
    Id add_repeating(int64_t now, int64_t interval, Callback callback)
    {
        std::unique_lock<std::mutex> lk(mutex_run_);
        Id id = nextId_++;
        timeouts_.insert({id, now + interval, interval, std::move(callback)});
        return id;
    }

    /**
     * Erase a given timeout event, returns true if the event was actually
     * erased and false if it didn't exist in our queue.
     */
    bool erase(Id id)
    {
        std::unique_lock<std::mutex> lk(mutex_run_);
        return timeouts_.get<BY_ID>().erase(id);
    }

    /**
     * Clear the queue.
     */
    void clear()
    {
        std::unique_lock<std::mutex> lk(mutex_run_);
        timeouts_.clear();
    }

    /**
     * Process all events that are due at times <= "now" by calling their
     * callbacks.
     *
     * Callbacks are allowed to call back into the queue and add / erase events;
     * they might create more events that are already due.  In this case,
     * run_once() will only go through the queue once, and return a "next
     * expiration" time in the past or present (<= now); run_loop()
     * will process the queue again, until there are no events already due.
     *
     * Note that it is then possible for run_loop to never return if
     * callbacks re-add themselves to the queue (or if you have repeating
     * callbacks with an interval of 0).
     *
     * Return the time that the next event will be due (same as
     * nextExpiration(), below)
     */
    int64_t run_once(int64_t now) {
        return run_internal(now, true);
    }
    int64_t run_loop(int64_t now) {
        return run_internal(now, false);
    }

    /**
     * Return the time that the next event will be due.
     */
    int64_t next_expiration() const
    {
        return (timeouts_.empty() ?
                std::numeric_limits<int64_t>::max() :
                timeouts_.get<BY_EXPIRATION>().begin()->expiration);
    }

private:
    TimeoutQueue(const TimeoutQueue&) = delete;
    TimeoutQueue& operator=(const TimeoutQueue&) = delete;

    struct Event {
        Id id;
        int64_t expiration;
        int64_t repeatInterval;
        Callback callback;
    };

    typedef boost::multi_index_container<
        Event,
        boost::multi_index::indexed_by<
            boost::multi_index::ordered_unique<
            boost::multi_index::member<Event, Id, &Event::id>>,
        boost::multi_index::ordered_non_unique<
            boost::multi_index::member<Event, int64_t, &Event::expiration>>>>
            Set;

    enum {
        BY_ID = 0,
        BY_EXPIRATION = 1,
    };

    Set        timeouts_;
    Id         nextId_;
    std::mutex mutex_run_;

    int64_t run_internal(int64_t now, bool onceOnly)
    {
        std::unique_lock<std::mutex> lk(mutex_run_);
        auto& byExpiration = timeouts_.get<BY_EXPIRATION>();
        int64_t nextExp;
        do {
            const auto end = byExpiration.upper_bound(now);
            std::vector<Event> expired;
            std::move(byExpiration.begin(), end, std::back_inserter(expired));
            byExpiration.erase(byExpiration.begin(), end);
            for (const auto& event : expired) {
                // Reinsert if repeating, do this before executing callbacks
                // so the callbacks have a chance to call erase
                if (event.repeatInterval >= 0) {
                    timeouts_.insert({event.id,
                                     now + event.repeatInterval,
                                     event.repeatInterval,
                                     event.callback});
                }
            }

            // Call callbacks
            for (const auto& event : expired) {
                event.callback(event.id, now);
            }
            nextExp = next_expiration();
        } while (!onceOnly && nextExp <= now);
        return nextExp;
    }
};

} /* namespace common */
