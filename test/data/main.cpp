#include <thread>
#include <chrono>
#include <string>

#include "common/log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "common/data.h"

auto   console = spdlog::stdout_color_mt("console");
size_t elt_size = 10;

void producer_th_func(common::data::Producer& p)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    log_info(console, "producer starting");
    for (uint8_t i = 0; i < 10; i++) {
        common::data::ByteBuffer buf(elt_size, i);
        p.push(common::data::type::us, buf);
    }
}

void consumer1_th_fun(common::data::Consumer& c)
{
    log_info(console, "consumer 1 starting");
    int ret;
    while(1) {
        common::data::ByteBuffer buf;
        ret = c.pop(common::data::type::us, buf);
        common_die_zero_void(console, ret, "error popping elt");
        log_info(console, "1 : {}", fmt::join(buf, "|"));
    }
}

void consumer2_th_fun(common::data::Consumer& c)
{
    log_info(console, "consumer 2 waiting...");
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    log_info(console, "consumer 2 starting");
    int chunk_nb = 0;
    int ret;
    while(1) {
        chunk_nb++;
        std::vector<common::data::ByteBuffer> chunk;
        ret = c.pop_chunk(common::data::type::us, 3, chunk);
        common_die_zero_void(console, ret, "error popping chunk");
        for (auto buf: chunk)
            log_info(console, "2 : chunk {} -> {}", chunk_nb, fmt::join(buf, "|"));
    }
}

int main()
{
    common::data::Handler h(console);
    common::data::Producer p(console, &h);
    common::data::Consumer c1(console, &h);
    common::data::Consumer c2(console, &h);

    h.reinit_queue(common::data::type::us, elt_size, 100);

    std::thread producer_th(producer_th_func, std::ref(p));
    std::thread consumer_th1(consumer1_th_fun, std::ref(c1));
    std::thread consumer_th2(consumer2_th_fun, std::ref(c2));

    producer_th.join();
    consumer_th1.join();
    consumer_th2.join();

    return 0;
}
