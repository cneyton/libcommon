#ifndef DATA_H
#define DATA_H

#include <map>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

#include <gsl/gsl>

#include "log.h"

/*
 * A queue that support several consumer requiring a copy of the data.
 * A shared pointer to each elt is stored into several queue. One per consumer.
 *
 * Supports popping chunks off the queue by returning a vector of elements
 */
namespace common
{

namespace data
{

constexpr size_t queue_max_size = 10000;

using ConsumerKey = int;

template<typename T>
class Queue
{
public:
    Queue(Logger logger): logger_(logger),
        q_map(std::map<ConsumerKey, std::queue<std::shared_ptr<T>>>()) {}
    Queue(Logger logger, size_t size): logger_(logger), max_size_(size),
        q_map(std::map<ConsumerKey, std::queue<std::shared_ptr<T>>>()) {}

    void push(const T& elt)
    {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto shr = std::make_shared<T>(elt);
            for (auto& pair: q_map) {
                if (pair.second.size() < max_size_)
                    pair.second.push(shr);
                else
                    log_warn(logger_, "queue exceeding {} elements, discarding data...",
                             queue_max_size);
            }
        }
        cond_.notify_all();
    }

    void push(T&& elt)
    {
        {
            std::unique_lock<std::mutex> lk(mutex_);
            auto shr = std::make_shared<T>(std::move(elt));
            for (auto& pair: q_map) {
                if (pair.second.size() < max_size_)
                    pair.second.push(shr);
                else
                    log_warn(logger_, "queue exceeding {} elements, discarding data...",
                             queue_max_size);
            }
        }
        cond_.notify_all();
    }

    //T pop(ConsumerKey& key)
    //{
        //std::unique_lock<std::mutex> lk(mutex_);

        //auto found = q_map.find(key);

        //if (found != q_map.end())

        //int idx = static_cast<int>(key);
        //if (idx < 0 || idx >= N)
            //common_die_throw_void(logger_, "invalid key");

        //cond_.wait(lk, [&] {return !q_map[idx].empty();});

        //auto shr = q_map[idx].front();
        //q_map[idx].pop();

        //T elt = T(*shr);
        //return elt;
    //}

    int pop(ConsumerKey& key, T& elt)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        auto search = q_map.find(key);
        if (search == q_map.end())
            common_die(logger_, -1, "invalid key");

        cond_.wait(lk, [&] {return !search->second.empty();});

        auto shr = search->second.front();
        search->second.pop();

        elt = T(*shr);
        return 0;
    }

    int pop_chunk(ConsumerKey key, size_t chunk_size, std::vector<T>& chunk)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        auto search = q_map.find(key);
        if (search == q_map.end())
            common_die(logger_, -1, "invalid key");

        cond_.wait(lk, [&] {return !(search->second.size() < chunk_size);});

        chunk.reserve(chunk_size);
        for (auto i = 0; i < chunk_size; i++) {
            auto shr = search->second.front();
            chunk.push_back(*shr);
            search->second.pop();
        }

        return 0;
    }

    ConsumerKey subscribe()
    {
        std::unique_lock<std::mutex> lk(mutex_);

        ConsumerKey key = static_cast<ConsumerKey>(nb_subsriber_);
        nb_subsriber_++;

        q_map[key] = std::queue<std::shared_ptr<T>>();
        return key;
    }

private:
    std::map<ConsumerKey, std::queue<std::shared_ptr<T>>> q_map;
    std::condition_variable  cond_;
    std::mutex               mutex_;
    size_t                   nb_subsriber_ = 0;
    size_t                   max_size_     = queue_max_size;

    Logger logger_;
};

template <typename T>
class Consumer
{
public:
    Consumer(Logger logger, Queue<T> * queue): logger_(logger), queue_(queue)
    {
        key_ = queue->subscribe();
    }

    int pop(T& elt)
    {
        int ret;
        ret = queue_->pop(key_, elt);
        common_die_zero(logger_, ret, -1, "consumer {} failed to pop elt", key_);
        return 0;
    }

    int pop_chunk(size_t chunk_size, std::vector<T>& chunk)
    {
        int ret;
        ret = queue_->pop_chunk(key_, chunk_size, chunk);
        common_die_zero(logger_, ret, -1, "consumer {} failed to pop chunk", key_);
        return 0;
    }

private:
    ConsumerKey key_;
    Queue<T>  * queue_;
    Logger      logger_;
};

template <typename T>
class Producer
{
public:
    Producer(Logger logger, Queue<T> * queue): logger_(logger), queue_(queue)
    {
    }

    void push(const T& elt)
    {
        queue_->push(elt);
    }

    void push(T&& elt)
    {
        queue_->push(std::move(elt));
    }

private:
    Queue<T> * queue_;
    Logger     logger_;
};

namespace us
{

template<typename T>
class Sample
{
public:
    Sample() {}
    Sample(T i, T q): i_(i), q_(q) {}
//private:
    T i_;
    T q_;
};

template<typename T>
class Frame
{
public:
    Frame() {}
    Frame(gsl::span<const uint8_t> span)
    {
        if (span.size() % sizeof(T) != 0)
            return;

        samples_.reserve(span.size()/sizeof(T));
        auto it = span.cbegin();
        while (it != span.cend()) {
            T i, q;
            std::copy(it, it + sizeof(T), (uint8_t*)&i);
            it += sizeof(T);
            std::copy(it, it + sizeof(T), (uint8_t*)&q);
            it += sizeof(T);
            samples_.emplace_back(i, q);
        }
    }

    std::vector<Sample<T>>& get_samples()
    {
        return samples_;
    }

private:
    std::vector<Sample<T>> samples_;
};

} /* namespace us */

//TODO: template for us_type, toco_type, etc
class Handler
{
public:
    Handler(Logger logger): logger_(logger), us_queue_(logger)
    {
    }

    Queue<us::Frame<int16_t>> * get_us_queue()
    {
        return &us_queue_;
    }

private:
    Queue<us::Frame<int16_t>> us_queue_;
    Logger logger_;
};

} /* namespace data */

} /* namespace common */

#endif /* DATA_H */
