#include <thread>
#include <chrono>
#include <string>

#include "common/log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "common/data_queue.h"

auto console = spdlog::stdout_color_mt("console");

void producer_th_func(common::DataQueue<int, 2>& q)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    log_info(console, "producer starting");
    std::vector<int> buf{1, 2, 3, 4, 5, 6, 7, 8};
    for (auto x: buf)
        q.push(x);
}

void consumer1_th_fun(common::DataQueue<int, 2>& q, uint16_t handle)
{
    log_info(console, "consumer 1 starting");
    while(1) {
        int x = q.pop(handle);
        log_info(console, "1 : {}", x);
    }
}

void consumer2_th_fun(common::DataQueue<int, 2>& q, uint16_t handle)
{
    log_info(console, "consumer 2 waiting...");
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    log_info(console, "consumer 2 starting");
    int chunk_nb = 0;
    while(1) {
        chunk_nb++;
        std::vector<int> chunk;
        q.pop_chunk(handle, 3, chunk);
        for (auto x: chunk)
            log_info(console, "2: chunk {} -> {}", chunk_nb, x);
    }
}

int main()
{
    common::DataQueue<int, 2> queue(console);
    auto handle1 = queue.subscribe();
    common_die_zero(console, handle1, -1, "handle 1 invalid");
    auto handle2 = queue.subscribe();

    std::thread producer_th(producer_th_func, std::ref(queue));
    std::thread consumer_th1(consumer1_th_fun, std::ref(queue), handle1);
    std::thread consumer_th2(consumer2_th_fun, std::ref(queue), handle2);

    producer_th.join();
    consumer_th1.join();
    consumer_th2.join();

    return 0;
}
