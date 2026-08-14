#include <algorithm>
#include <cstring>
#include <cassert>
#include <type_traits>
#include <vector>

#include "types.h"
#include "parser.h"

namespace {
    template <typename T>
    T read_field(const std::byte*& cursor) {
        T val;
        std::memcpy(&val, cursor, sizeof(T));
        cursor += sizeof(T);

        if constexpr (std::is_same_v<T, uint16_t>) val = be16toh(val);
        else if constexpr (std::is_same_v<T, uint32_t>) val = be32toh(val);
        else if constexpr (std::is_same_v<T, uint64_t>) val = be64toh(val);

        return val;
    }
}

AddOrderMessage parser::parse_add_order(const std::byte* cursor) {
    AddOrderMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);
    msg.side = read_field<Side>(cursor);
    msg.shares = read_field<uint32_t>(cursor);
    msg.stock = read_field<Ticker>(cursor);
    msg.price = read_field<uint32_t>(cursor);

    return msg;
}

AddOrderMPIDAttributionMessage parser::parse_add_order_mpid(const std::byte* cursor) {
    AddOrderMPIDAttributionMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);
    msg.side = read_field<Side>(cursor);
    msg.shares = read_field<uint32_t>(cursor);
    msg.stock = read_field<Ticker>(cursor);
    msg.price = read_field<uint32_t>(cursor);
    msg.MPID = read_field<uint32_t>(cursor);

    return msg;
}

OrderExecutedMessage parser::parse_order_executed(const std::byte* cursor) {
    OrderExecutedMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);
    msg.executed_shares = read_field<uint32_t>(cursor);
    msg.match_number = read_field<uint64_t>(cursor);

    return msg;
};

OrderExecutedPriceMessage parser::parse_order_executed_price(const std::byte* cursor) {
    OrderExecutedPriceMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);
    msg.executed_shares = read_field<uint32_t>(cursor);
    msg.match_number = read_field<uint64_t>(cursor);
    msg.printable = read_field<char>(cursor);
    msg.execution_price = read_field<uint32_t>(cursor);

    return msg;
};

OrderCancelMessage parser::parse_order_cancel(const std::byte* cursor) {
    OrderCancelMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);
    msg.cancelled_shares = read_field<uint32_t>(cursor);

    return msg;
};

OrderDeleteMessage parser::parse_order_delete(const std::byte* cursor) {
    OrderDeleteMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);

    return msg;
};

OrderReplaceMessage parser::parse_order_replace(const std::byte* cursor) {
    OrderReplaceMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.original_order_reference_number = read_field<uint64_t>(cursor);
    msg.new_order_reference_number = read_field<uint64_t>(cursor);
    msg.shares = read_field<uint32_t>(cursor);
    msg.price = read_field<uint32_t>(cursor);

    return msg;
};

TradeMessage parser::parse_trade(const std::byte* cursor) {
    TradeMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.order_reference_number = read_field<uint64_t>(cursor);
    msg.side = read_field<Side>(cursor);
    msg.shares = read_field<uint32_t>(cursor);
    msg.stock = read_field<Ticker>(cursor);
    msg.price = read_field<uint32_t>(cursor);
    msg.match_number = read_field<uint64_t>(cursor);

    return msg;
};

CrossTradeMessage parser::parse_cross_trade(const std::byte* cursor) {
    CrossTradeMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.shares = read_field<uint64_t>(cursor);
    msg.stock = read_field<Ticker>(cursor);
    msg.cross_price = read_field<uint32_t>(cursor);
    msg.match_number = read_field<uint64_t>(cursor);
    msg.cross_type = read_field<char>(cursor);

    return msg;
};

BrokenTradeMessage parser::parse_broken_trade(const std::byte* cursor) {
    BrokenTradeMessage msg;

    msg.stock_locate = read_field<uint16_t>(cursor);
    msg.tracking_number = read_field<uint16_t>(cursor);
    msg.timestamp = read_field<Timestamp>(cursor);
    msg.match_number = read_field<uint64_t>(cursor);

    return msg;
};

ITCHReader::ITCHReader(const std::string& filepath) {
    file.open(filepath, std::ios::in | std::ios::binary);

    if (!file)
        std::cout << "Could not open file: " << filepath << std::endl;
};

ITCHReader::~ITCHReader() {
    if (file) 
        file.close();
};

void ITCHReader::read_messages(std::function<void(Message&& msg)> process, uint64_t& counter) {
    std::array<std::byte, BUFFER_SIZE> buffer;
    std::byte* cursor = buffer.data();
    std::byte* buf_end = buffer.data();

    // Decodes one message at cursor (length prefix + type byte already known to be present)
    // and advances cursor past it. Shared by the lazy in-stream loop and the exact
    // end-of-file drain below so the dispatch switch isn't duplicated between them.
    auto process_next_message = [&process, &counter](std::byte*& cursor) {
        // Byte order for NASDAQ sample files is big-endian
        // First two bytes represent the length of the next message
        uint16_t len_be;
        std::memcpy(&len_be, cursor, LENGTH_BYTES);
        uint16_t len = be16toh(len_be);

        cursor += LENGTH_BYTES;

        // Next byte is a char representing the type of message
        // Endianness does not matter for single bytes
        uint8_t message_type = std::to_integer<uint8_t>(*cursor);

        ++cursor;

        // Consider changing switch statement to dispatch table to fix branch misses if bottleneck
        // Only actively processing add, execute, replace, cancel, delete messages
        switch (message_type)
        {
            case MessageType::AddOrder:
            {
                AddOrderMessage msg = parser::parse_add_order(cursor);
                process(msg);
                break;
            }
            case MessageType::AddOrderMPIDAttribution:
            {
                AddOrderMPIDAttributionMessage msg = parser::parse_add_order_mpid(cursor);
                process(msg);
                break;
            }
            case MessageType::OrderExecuted:
            {
                OrderExecutedMessage msg = parser::parse_order_executed(cursor);
                process(msg);
                break;
            }
            case MessageType::OrderExecutedPrice:
            {
                OrderExecutedPriceMessage msg = parser::parse_order_executed_price(cursor);
                process(msg);
                break;
            }
            case MessageType::OrderCancel:
            {
                OrderCancelMessage msg = parser::parse_order_cancel(cursor);
                process(msg);
                break;
            }
            case MessageType::OrderDelete:
            {
                OrderDeleteMessage msg = parser::parse_order_delete(cursor);
                process(msg);
                break;
            }
            case MessageType::OrderReplace:
            {
                OrderReplaceMessage msg = parser::parse_order_replace(cursor);
                process(msg);
                break;
            }
#ifdef BUILD_PARQUET_EXPORT
            case MessageType::Trade:
            {
                TradeMessage msg = parser::parse_trade(cursor);
                process(msg);
                break;
            }
            case MessageType::CrossTrade:
            {
                CrossTradeMessage msg = parser::parse_cross_trade(cursor);
                process(msg);
                break;
            }
            case MessageType::BrokenTrade:
            {
                BrokenTradeMessage msg = parser::parse_broken_trade(cursor);
                process(msg);
                break;
            }
#endif
            default:
                break;
        }
        ++counter;
        cursor += len - 1;  // Move cursor to first byte of next message
    };

    while (true)
    {
        // Move leftover unread bytes at the end of the buffer to the start
        std::size_t leftover = buf_end - cursor;
        std::memmove(buffer.data(), cursor, leftover);
        // Reset cursor and end pointers
        cursor = buffer.data();
        buf_end = buffer.data() + leftover;

        // Fill the remainder of the buffer
        file.read(reinterpret_cast<char*>(buf_end), BUFFER_SIZE - leftover);
        std::size_t bytes_read = file.gcount();
        if (bytes_read == 0)
        {
            // End of file: no further reads will bring in the rest of a message, so drain
            // whatever complete messages remain exactly, without the lazy MAX_MSG_SIZE
            // margin below (which assumes more data may still arrive).
            while (cursor + LENGTH_BYTES <= buf_end)
            {
                uint16_t len_be;
                std::memcpy(&len_be, cursor, LENGTH_BYTES);
                uint16_t len = be16toh(len_be);

                if (cursor + LENGTH_BYTES + len > buf_end)
                    break; // trailing partial message, nothing more can be decoded

                process_next_message(cursor);
            }
            break;
        }
        buf_end += bytes_read;

        // Stop once the cursor is within the maximum message length + bytes for the length
        // We lose out on at most 2-3 messages per iteration through the buffer using this lazier stopping criterion
        while (cursor + MAX_MSG_SIZE + LENGTH_BYTES <= buf_end)
        {
            process_next_message(cursor);
        }
    }
};


Timestamp convertIntegerToTimestamp(const uint64_t value) {
    Timestamp time{};
    std::memcpy(time.data(), &value, std::min(sizeof(value), sizeof(time)));
    return time;
}

Ticker convertStringToTicker(const std::string& name) {
    Ticker ticker{};
    std::copy(name.begin(), 
              name.begin() + std::min(name.length(), ticker.size()),
              ticker.begin());
    return ticker;
} 

std::vector<std::byte> parser::build_add_order_bytes(uint64_t timestamp, uint64_t order_reference_number, Side side, uint32_t shares, std::string stock, uint32_t price) {
    AddOrderMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.order_reference_number = htobe64(order_reference_number);
    msg.side = side;
    msg.shares = htobe32(shares);
    msg.stock = convertStringToTicker(stock);
    msg.price = htobe32(price);

    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}

std::vector<std::byte> parser::build_add_order_mpid_bytes(uint64_t timestamp, uint64_t order_reference_number, Side side, uint32_t shares, std::string stock, uint32_t price, uint32_t MPID) {
    AddOrderMPIDAttributionMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.order_reference_number = htobe64(order_reference_number);
    msg.side = side;
    msg.shares = htobe32(shares);
    msg.stock = convertStringToTicker(stock);
    msg.price = htobe32(price);
    msg.MPID = htobe32(MPID);

    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}

std::vector<std::byte> parser::build_execute_order_bytes(uint64_t timestamp, uint64_t order_reference_number, uint32_t executed_shares, uint64_t match_number) {
    OrderExecutedMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.order_reference_number = htobe64(order_reference_number);
    msg.executed_shares = htobe32(executed_shares);
    msg.match_number = htobe64(match_number);

    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}

std::vector<std::byte> parser::build_execute_price_order_bytes(uint64_t timestamp, uint64_t order_reference_number, uint32_t executed_shares, uint64_t match_number, uint32_t execution_price) {
    OrderExecutedPriceMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.order_reference_number = htobe64(order_reference_number);
    msg.executed_shares = htobe32(executed_shares);
    msg.match_number = htobe64(match_number);
    msg.execution_price = htobe32(execution_price);
    
    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}

std::vector<std::byte> parser::build_cancel_order_bytes(uint64_t timestamp, uint64_t order_reference_number, uint32_t cancelled_shares) {
    OrderCancelMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.order_reference_number = htobe64(order_reference_number);
    msg.cancelled_shares = htobe32(cancelled_shares);

    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}

std::vector<std::byte> parser::build_replace_order_bytes(uint64_t timestamp, uint64_t original_id, uint64_t new_id, uint32_t shares, uint32_t price) {
    OrderReplaceMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.original_order_reference_number = htobe64(original_id);
    msg.new_order_reference_number = htobe64(new_id);
    msg.shares = htobe32(shares);
    msg.price = htobe32(price);

    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}

std::vector<std::byte> parser::build_delete_order_bytes(uint64_t timestamp, uint64_t order_reference_number) {
    OrderDeleteMessage msg{};
    msg.timestamp = convertIntegerToTimestamp(timestamp);
    msg.order_reference_number = htobe64(order_reference_number);

    std::vector<std::byte> raw_bytes{sizeof(msg)};
    std::memcpy(&raw_bytes[0], &msg, sizeof(msg));
    return raw_bytes;
}
