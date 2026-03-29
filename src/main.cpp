#include <array>
#include <cassert>
#include <cmath>
#include <chrono>
#include <cstring>
#include <cstdint>
#include <endian.h>
#include <fstream>
#include <iostream>
#include <string>

#include "types.h"
#include "parser.h"
#include "lob.h"


int main()
{   
    auto start = std::chrono::high_resolution_clock::now();

    // const std::string path = "/mnt/d/01302020.NASDAQ_ITCH50";
    const std::string path = "./sample1billion.NASDAQ_ITCH50";
    ITCHReader reader{path};
    OrderbookManager orderbook_manager{};
    uint64_t counter{};

    reader.read_messages([&orderbook_manager](const Message& msg) {
        orderbook_manager.process(msg);
    }, counter);
    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start).count();

    std::cout << "Messages: " << counter << '\n';
    std::cout << "Time: " << duration << "ms\n";
    std::cout << "Throughput: " << (counter * 1000) / duration << " msg/s\n";
    std::cout << "Efficiency: " << (duration_ns / counter) << " ns/msg\n"; 

    return 0;
}