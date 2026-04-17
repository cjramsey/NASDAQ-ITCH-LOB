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

Timestamp convertIntegerToTimestamp(const uint64_t value);

Ticker convertStringToTicker(const std::string& name);

namespace parser {

AddOrderMessage parse_add_order(const std::byte* cursor);
AddOrderMPIDAttributionMessage parse_add_order_mpid(const std::byte* cursor);
OrderExecutedMessage parse_order_executed(const std::byte* cursor);
OrderExecutedPriceMessage parse_order_executed_price(const std::byte* cursor);
OrderCancelMessage parse_order_cancel(const std::byte* cursor);
OrderReplaceMessage parse_order_replace(const std::byte* cursor);
OrderDeleteMessage parse_order_delete(const std::byte* cursor);

std::vector<std::byte> build_add_order_bytes(uint64_t timestamp, uint64_t order_reference_number, Side side, uint32_t shares, std::string stock, uint32_t price);
std::vector<std::byte> build_add_order_mpid_bytes(uint64_t timestamp, uint64_t order_reference_number, Side side, uint32_t shares, std::string stock, uint32_t price, uint32_t MPID);
std::vector<std::byte> build_execute_order_bytes(uint64_t timestamp, uint64_t order_reference_number, uint32_t executed_shares, uint64_t match_number);
std::vector<std::byte> build_execute_price_order_bytes(uint64_t timestamp, uint64_t order_reference_number, uint32_t executed_shares, uint64_t match_number, uint32_t execution_price);
std::vector<std::byte> build_cancel_order_bytes(uint64_t timestamp, uint64_t order_reference_number, uint32_t cancelled_shares);
std::vector<std::byte> build_replace_order_bytes(uint64_t timestamp, uint64_t original_id, uint64_t new_id, uint32_t shares, uint32_t price);
std::vector<std::byte> build_delete_order_bytes(uint64_t timestamp, uint64_t order_reference_number);

};