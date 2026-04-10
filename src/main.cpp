#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>
#include <thread>

#include "types.h"
#include "parser.h"
#include "lob.h"
#include "ring_buffer.h"


int main()
{   
    // const std::string path = "/mnt/d/01302020.NASDAQ_ITCH50";
    const std::string path = "./data/sample1billion.NASDAQ_ITCH50";
    ITCHReader reader{path};
    OrderbookManager orderbook_manager{};
    uint64_t counter{};
    std::atomic<bool> running{true};
    auto start = std::chrono::high_resolution_clock::now();

#ifdef USE_RING_BUFFER

    constexpr std::size_t N = 1 << 12;
    SPSCRingBuffer<N> ring_buffer{};

    auto ring_buffer_handler = [&ring_buffer](Message&& msg) {
        while (!ring_buffer.push(std::move(msg)));
    };

    std::thread t1{[&reader, &ring_buffer_handler, &counter, &running]() {
        reader.read_messages(ring_buffer_handler, counter);
        running.store(false, std::memory_order_release);
    }};

    std::thread t2{[&orderbook_manager, &ring_buffer, &running](){
        Message msg;
        while (!ring_buffer.empty() || running.load(std::memory_order_acquire)) {
            if (ring_buffer.pop(msg)) {
                orderbook_manager.process(msg);
            }
        }
    }};

    t1.join();
    t2.join();

#else

    auto direct_handler = [&orderbook_manager](Message&& msg) {
        orderbook_manager.process(msg);
    };

    reader.read_messages(direct_handler, counter);
    
#endif

    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    std::cout << "Messages: " << counter << '\n';
    std::cout << "Time: " << duration << "ms\n";
    std::cout << "Throughput: " << (counter * 1000) / duration << " msg/s\n";
    std::cout << "Efficiency: " << (duration_ns / counter) << " ns/msg\n"; 

    return 0;
}