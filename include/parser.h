#pragma once

#include <fstream>
#include <functional>

#include "types.h"

static constexpr size_t BUFFER_SIZE = 1 << 16;
static constexpr size_t MAX_MSG_SIZE = 50;
static constexpr size_t LENGTH_BYTES = 2; 


// Class for reading binary data from sample files
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
