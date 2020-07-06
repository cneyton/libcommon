#pragma once

#include <vector>
#include <map>

#include "log.h"
#include "readerwriterqueue.h"

namespace common
{

namespace data
{

class data_error: public std::runtime_error
{
public:
    data_error(const std::string& what_arg): std::runtime_error(what_arg) {}
};

class Handler: public Log
{
public:
    Handler(Logger logger): Log(logger) {}

    bool add_queue(std::string& type, size_t max_size=default_queue_size)
    {
        auto [it, success] = map_.emplace(type, max_size);
        return success;
    }

    void reinit_queue(std::string& type)
    {
        auto found = map_.find(type);
        if (found != map_.end()) {
            while(found->second.pop()) {};
        } else {
            log_warn(logger_, "{} queue not found", type);
        }
    }

    /*
     * Callback called when data is pushed.
     * Should be redefined in the application.
     * ex: this cb can be used to trigger the pipeline by setting all the filters to ready
     */
    virtual int data_pushed() {return 0;}

    /*
     * Callback called when eof is reached.
     * Should be redefined in the application.
     * ex: this cb can be used to stop the pipeline
     */
    virtual int eof()         {return 0;}

    void push(std::string& type, std::string_view v)
    {
        auto found = map_.find(type);
        if (found != map_.end()) {
            if (!found->second.try_enqueue(std::string(v)))
                log_warn(logger_, "{} queue full, discarding data", type);
        } else {
            log_warn(logger_, "{} queue not found", type);
        }
        data_pushed();
    }

    bool pop(std::string type, std::string& buf)
    {
        bool ret = false;
        auto found = map_.find(type);
        if (found != map_.end()) {
            ret = found->second.try_dequeue(buf);
        } else {
            log_warn(logger_, "{} queue not found", type);
        }
        return ret;
    }

    bool pop_chunk(std::string& type, size_t chunk_size, std::vector<std::string>& chunk)
    {
        bool ret = false;
        auto found = map_.find(type);
        if (found != map_.end()) {
            log_warn(logger_, "{} queue not found", type);
            return ret;
        }

        if (found->second.size_approx() < chunk_size)
            return ret;

        std::string buf;
        size_t i = 0;
        while (found->second.try_dequeue(buf) && i++ < chunk_size) {
            chunk.push_back(std::move(buf));
        }
        return true;
    }

private:
    static constexpr size_t default_queue_size = 1000;
    std::map<std::string, ReaderWriterQueue<std::string>> map_;
};

} /* namespace data */

} /* namespace common */
