#pragma once

#include <fstream>
#include <functional>

#include "types.h"


constexpr size_t powi(uint64_t base, unsigned int exp)
{
    size_t res = 1;
    while (exp) {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        base *= base;
    }
    return res;
}


static constexpr size_t BUFFER_SIZE = powi(2, 20);
static constexpr size_t MAX_MSG_SIZE = 50;


class ITCHReader
{
public:
    ITCHReader() = delete;

    ITCHReader(const std::string& filepath);

    ITCHReader(const ITCHReader&) = delete;
    ITCHReader& operator=(const ITCHReader&) = delete;

    ~ITCHReader();

    void read_messages(std::function<void(Message&& msg)> process, uint64_t& counter);

private:
    std::ifstream file;
};


namespace parser {

AddOrderMessage parse_add_order(const std::byte* cursor);
AddOrderMPIDAttributionMessage parse_add_order_mpid(const std::byte* cursor);
OrderExecutedMessage parse_order_executed(const std::byte* cursor);
OrderExecutedPriceMessage parse_order_executed_price(const std::byte* cursor);
OrderCancelMessage parse_order_cancel(const std::byte* cursor);
OrderReplaceMessage parse_order_replace(const std::byte* cursor);
OrderDeleteMessage parse_order_delete(const std::byte* cursor);

}
