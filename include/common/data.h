#ifndef DATA_H
#define DATA_H

#include <vector>
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

constexpr size_t queue_max_size = 1000;

using ConsumerKey = int;
using ByteBuffer  = std::vector<uint8_t>;
using View        = gsl::span<const uint8_t>;
using ShrdQueue   = std::queue<std::shared_ptr<ByteBuffer>>;

class Queue
{
public:
    Queue(Logger logger, size_t elt_size): logger_(logger), elt_size_(elt_size),
        q_map_(std::map<ConsumerKey, ShrdQueue>()) {}
    Queue(Logger logger, size_t elt_size, size_t max_size): logger_(logger), elt_size_(elt_size),
        max_size_(max_size), q_map_(std::map<ConsumerKey, ShrdQueue>()) {}

    virtual ~Queue() {};

    int push(const gsl::span<const uint8_t> span)
    {
        if (static_cast<size_t>(span.size()) != elt_size_) {
            common_die(logger_, -1, "invalid size : {} != {}", span.size(), elt_size_);
        }

        {
            std::unique_lock<std::mutex> lk(mutex_);
            ByteBuffer buf(span.cbegin(), span.cend());
            auto shr = std::make_shared<ByteBuffer>(buf);
            for (auto& pair: q_map_) {
                if (pair.second.size() < max_size_)
                    pair.second.push(shr);
                else
                    log_warn(logger_, "queue exceeding {} elements, discarding data...", max_size_);
            }
        }
        cond_.notify_all();
        return 0;
    }

    int pop(ConsumerKey& key, ByteBuffer& buffer)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        auto search = q_map_.find(key);
        if (search == q_map_.end())
            common_die(logger_, -1, "invalid key");

        cond_.wait(lk, [&] {return !search->second.empty();});

        auto shr = search->second.front();
        search->second.pop();

        buffer = ByteBuffer(*shr);
        return 0;
    }

    int pop_chunk(ConsumerKey& key, size_t chunk_size, std::vector<ByteBuffer>& chunk)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        auto search = q_map_.find(key);
        if (search == q_map_.end())
            common_die(logger_, -1, "invalid key");

        cond_.wait(lk, [&] {return !(search->second.size() < chunk_size);});

        chunk.reserve(chunk_size);
        for (size_t i = 0; i < chunk_size; i++) {
            auto shr = search->second.front();
            chunk.push_back(*shr);
            search->second.pop();
        }

        return 0;
    }

    int subscribe(ConsumerKey key)
    {
        std::unique_lock<std::mutex> lk(mutex_);

        auto search = q_map_.find(key);
        if (search != q_map_.end())
            common_die(logger_, -1, "key already in use");

        q_map_[key] = ShrdQueue();
        return 0;
    }

private:
    Logger logger_;
    size_t elt_size_;
    size_t max_size_     = queue_max_size;

    std::map<ConsumerKey, ShrdQueue> q_map_;
    std::condition_variable          cond_;
    std::mutex                       mutex_;
};

enum class type {
    us,
    toco,
    oxy
};

class Handler: public Log
{
public:
    Handler(Logger logger): Log(logger), us_queue_(std::make_unique<Queue>(logger, 0)) {}

    ConsumerKey add_consumer()
    {
        nb_consumer_++;
        ConsumerKey key = static_cast<ConsumerKey>(nb_consumer_);
        keys_.push_back(key);
        return key;
    }

    int reinit_queue(type t, size_t elt_size, size_t max_size)
    {
        switch (t) {
        case type::us:
            us_queue_ = std::make_unique<Queue>(logger_, elt_size, max_size);
            // resubsribe the consumers
            for (auto& key: keys_) {
                us_queue_->subscribe(key);
            }
            break;
        default:
            common_die(logger_, -2, "invalid type, you should not be here");
        }
        return 0;
    }

    int push(type t, const View& v)
    {
        int ret;
        switch (t) {
        case type::us:
            ret = us_queue_->push(v);
            common_die_zero(logger_, ret, -1, "failed to push data to us queue");
            break;
        default:
            common_die(logger_, -2, "invalid type, you should not be here");
        }
        return 0;
    }

    int pop(type t, ConsumerKey key, ByteBuffer& buf)
    {
        int ret;
        switch (t) {
        case type::us:
            ret = us_queue_->pop(key, buf);
            common_die_zero(logger_, ret, -1, "failed to pop data from us queue");
            break;
        default:
            common_die(logger_, -2, "invalid type, you should not be here");
        }
        return 0;
    }

    int pop_chunk(type t, ConsumerKey& key, size_t chunk_size, std::vector<ByteBuffer>& chunk)
    {
        int ret;
        switch (t) {
        case type::us:
            ret = us_queue_->pop_chunk(key, chunk_size, chunk);
            common_die_zero(logger_, ret, -1, "failed to pop chunk from us queue");
            break;
        default:
            common_die(logger_, -2, "invalid type, you should not be here");
        }
        return 0;
    }


private:
    uint16_t nb_consumer_ = 0;
    std::vector<ConsumerKey> keys_;

    // TODO: maybe change all the queues with a vector or map
    std::unique_ptr<Queue> us_queue_;
};

class Producer: virtual public Log
{
public:
    Producer(Logger logger, Handler * h): Log(logger), h_(h) {}

    int push(type t, const View& v)
    {
        int ret;
        ret = h_->push(t, v);
        common_die_zero(logger_, ret, -1, "producer failed to push buffer");
        return 0;
    }

private:
    Handler * h_;
};

class Consumer: virtual public Log
{
public:
    Consumer(Logger logger, Handler * h): Log(logger), h_(h)
    {
        key_ = h_->add_consumer();
    }

    int pop(type t, ByteBuffer& buf)
    {
        int ret;
        ret = h_->pop(t, key_, buf);
        common_die_zero(logger_, ret, -1, "consumer {} failed to pop elt", key_);
        return 0;
    }

    int pop_chunk(type t, size_t chunk_size, std::vector<ByteBuffer>& chunk)
    {
        int ret;
        ret = h_->pop_chunk(t, key_, chunk_size, chunk);
        common_die_zero(logger_, ret, -1, "consumer {} failed to pop chunk", key_);
        return 0;
    }

private:
    Handler     * h_;
    ConsumerKey   key_;
};

} /* namespace data */

} /* namespace common */

#endif /* DATA_H */
