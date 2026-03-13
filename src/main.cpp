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
    const std::string path = "./sample100million.NASDAQ_ITCH50";
    ITCHReader reader{path};
    OrderbookManager orderbook_manager{};

    reader.read_messages([&orderbook_manager](const Message& msg) {
        orderbook_manager.process(msg);
    });
    
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    std::cout << duration.count() << std::endl;

    return 0;
}