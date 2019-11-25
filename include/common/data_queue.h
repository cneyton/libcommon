#ifndef DATA_QUEUE_H
#define DATA_QUEUE_H

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

#include "log.h"

/*
 * A queue that support several consumer requiring a copy of the data.
 * A shared pointer to each elt is stored into several queue. One per consumer.
 *
 * Supports popping chunks off the queue by returning a vector of elements
 */
namespace common
{

constexpr size_t data_queue_max_size = 100;

using ConsumerHandle = int;

template<typename T, size_t N>
class DataQueue
{
public:
    DataQueue(Logger logger): logger_(logger),
        queues_(std::array<std::queue<std::shared_ptr<T>>, N>()) {}
    DataQueue(Logger logger, size_t size): logger_(logger), max_size_(size),
        queues_(std::array<std::queue<std::shared_ptr<T>>, N>()) {}

    void push(const T& elt)
    {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto shr = std::make_shared<T>(elt);
            for (auto& queue: queues_) {
                if (queue.size() < max_size_)
                    queue.push(shr);
                else
                    log_warn(logger_, "queue exceeding {} elements, discarding data...",
                             data_queue_max_size);
            }
        }
        cond_.notify_all();
    }

    void push(T&& elt)
    {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto shr = std::make_shared<T>(std::move(elt));
            for (auto& queue: queues_) {
                if (queue.size() < max_size_)
                    queue.push(shr);
                else
                    log_warn(logger_, "queue exceeding {} elements, discarding data...",
                             data_queue_max_size);
            }
        }
        cond_.notify_all();
    }

    T pop(ConsumerHandle handle)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        int idx = static_cast<int>(handle);
        if (idx < 0 || idx >= N)
            common_die_throw_void(logger_, "invalid handle");

        cond_.wait(lk, [&] {return !queues_[idx].empty();});

        auto shr = queues_[idx].front();
        queues_[idx].pop();

        T elt = T(*shr);
        return elt;
    }

    int pop(ConsumerHandle handle, T& elt)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        int idx = static_cast<int>(handle);
        if (idx < 0 || idx >= N)
            common_die(logger_, -1, "invalid handle");

        cond_.wait(lk, [&] {return !queues_[idx].empty();});

        auto shr = queues_[idx].front();
        queues_[idx].pop();

        elt = T(*shr);
        return 0;
    }

    int pop_chunk(ConsumerHandle handle, size_t chunk_size, std::vector<T>& chunk)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        int idx = static_cast<int>(handle);
        if (idx < 0 || idx >= N)
            common_die(logger_, -1, "invalid handle");

        cond_.wait(lk, [&] {return !(queues_[idx].size() < chunk_size);});

        chunk.reserve(chunk_size);
        for (auto i = 0; i < chunk_size; i++) {
            auto shr = queues_[idx].front();
            chunk.push_back(*shr);
            queues_[idx].pop();
        }

        return 0;
    }

    ConsumerHandle subscribe()
    {
        std::unique_lock<std::mutex> lk(mutex_);
        if (nb_subsriber_ >= N)
            common_die_throw_void(logger_, "all handles are used");
        ConsumerHandle handle = static_cast<ConsumerHandle>(nb_subsriber_);
        nb_subsriber_++;
        return handle;
    }

private:
    std::array<std::queue<std::shared_ptr<T>>, N> queues_;
    std::condition_variable  cond_;
    std::mutex               mutex_;
    size_t                   nb_subsriber_ = 0;
    size_t                   max_size_     = data_queue_max_size;

    Logger logger_;
};

} /* namespace common */

#endif
