#pragma once

#include <vector>
#include <map>

#include "log.h"
#include "readerwriterqueue.h"

namespace common
{

namespace data
{

namespace oxy
{

class Frame
{
public:
    Frame(const std::string_view&);

private:
    using WaveformData = std::array<char, 8>;
    friend std::ostream& operator<<(std::ostream& os, const Frame& f);

    struct POD {
        int16_t  spo2;
        int16_t  pulse_rate;
        uint16_t perfusion_index;
        uint16_t pvi;
        std::array<WaveformData, 63> waveforms;
    }__attribute__((packed)) pod_;
};

} /* namespace oxy */

namespace us
{

template<typename T>
struct IQ
{
    using elem_type = T;
    T i; T q;
};

class Frame
{
public:
    Frame();
    virtual ~Frame();

private:
};

} /* namespace us */

namespace toco
{

class Frame
{
public:
    Frame(const std::string_view&);

private:
    using Sample = uint16_t;
    static constexpr uint8_t nb_samples = 4;

    struct POD {
        std::array<Sample, nb_samples> waveform;
    }__attribute__((packed)) pod_;
};

} /* namespace toco */

class data_error: public std::runtime_error
{
public:
    data_error(const std::string& what_arg): std::runtime_error(what_arg) {}
};

class Handler: public Log
{
public:
    Handler(Logger logger): Log(logger) {}

    bool add_queue(std::string_view type, size_t max_size=default_queue_size)
    {
        auto [it_queue, success_queue] = queues_.emplace(type, max_size);
        auto [it_eof, success_eof] = eofs_.emplace(type, false);
        return success_queue && success_eof;
    }

    void reinit_queue(std::string_view type)
    {
        auto found = queues_.find(std::string(type));
        if (found != queues_.end()) {
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

    void push(std::string_view type, std::string_view v)
    {
        auto found = queues_.find(std::string(type));
        if (found != queues_.end()) {
            if (!found->second.try_enqueue(std::string(v)))
                log_warn(logger_, "{} queue full, discarding data", type);
        } else {
            log_warn(logger_, "{} queue not found", type);
        }
        data_pushed();
    }

    void set_eof(std::string_view type)
    {
        auto found = eofs_.find(std::string(type));
        if (found == eofs_.end())
            throw data_error(fmt::format("{} eof not found", type));
        found->second = true;
    }

    void reset_eof(std::string_view type)
    {
        auto found = eofs_.find(std::string(type));
        if (found == eofs_.end())
            throw data_error(fmt::format("{} eof not found", type));
        found->second = false;
    }

    bool pop(std::string_view type, std::string& buf)
    {
        bool ret = false;
        auto found = queues_.find(std::string(type));
        if (found != queues_.end()) {
            ret = found->second.try_dequeue(buf);
        } else {
            log_warn(logger_, "{} queue not found", type);
        }
        return ret;
    }

    // 0 = not enought data
    // 1 = data retreived
    // 2 = eof
    int pop_chunk(std::string_view type, size_t chunk_size, std::vector<std::string>& chunk)
    {
        auto found = queues_.find(std::string(type));
        if (found == queues_.end()) {
            throw data_error(fmt::format("{} queue not found", type));
        }

        if (found->second.size_approx() < chunk_size) {
            auto found_eof = eofs_.find(std::string(type));
            if (found_eof == eofs_.end()) {
                throw data_error(fmt::format("{} eof not found", type));
            }
            if (found_eof->second)
                return 2;
            else
                return 0;
        }

        std::string buf;
        size_t i = 0;
        while (found->second.try_dequeue(buf) && i++ < chunk_size) {
            chunk.push_back(std::move(buf));
        }
        return 1;
    }

private:
    static constexpr size_t default_queue_size = 1000;
    std::map<std::string, ReaderWriterQueue<std::string>> queues_;
    std::map<std::string, bool>                           eofs_;
};

} /* namespace data */
} /* namespace common */
