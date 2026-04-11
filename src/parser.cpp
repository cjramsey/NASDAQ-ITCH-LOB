#include <cstring>
#include <cassert>

#include "types.h"
#include "parser.h"
#include "lob.h"

AddOrderMessage parser::parse_add_order(const std::byte* cursor) {
    AddOrderMessage msg;

    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.order_reference_number, cursor, sizeof(msg.order_reference_number));
    msg.order_reference_number = be64toh(msg.order_reference_number);
    cursor += sizeof(msg.order_reference_number);

    std::memcpy(&msg.side, cursor, sizeof(msg.side));
    cursor += sizeof(msg.side);

    std::memcpy(&msg.shares, cursor, sizeof(msg.shares));
    msg.shares = be32toh(msg.shares);
    cursor += sizeof(msg.shares);

    std::memcpy(msg.stock.data(), cursor, sizeof(msg.stock));
    cursor += sizeof(msg.stock);

    std::memcpy(&msg.price, cursor, sizeof(msg.price));
    msg.price = be32toh(msg.price);

    return msg;
}

AddOrderMPIDAttributionMessage parser::parse_add_order_mpid(const std::byte* cursor) {
    AddOrderMPIDAttributionMessage msg;

    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.order_reference_number, cursor, sizeof(msg.order_reference_number));
    msg.order_reference_number = be64toh(msg.order_reference_number);
    cursor += sizeof(msg.order_reference_number);

    std::memcpy(&msg.side, cursor, sizeof(msg.side));
    cursor += sizeof(msg.side);

    std::memcpy(&msg.shares, cursor, sizeof(msg.shares));
    msg.shares = be32toh(msg.shares);
    cursor += sizeof(msg.shares);

    std::memcpy(msg.stock.data(), cursor, sizeof(msg.stock));
    cursor += sizeof(msg.stock);

    std::memcpy(&msg.price, cursor, sizeof(msg.price));
    msg.price = be32toh(msg.price);
    cursor += sizeof(msg.price);

    std::memcpy(&msg.MPID, cursor, sizeof(msg.MPID));
    msg.MPID = be32toh(msg.MPID);

    return msg;
}

OrderExecutedMessage parser::parse_order_executed(const std::byte* cursor) {
    OrderExecutedMessage msg;

    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.order_reference_number, cursor, sizeof(msg.order_reference_number));
    msg.order_reference_number = be64toh(msg.order_reference_number);
    cursor += sizeof(msg.order_reference_number);

    std::memcpy(&msg.executed_shares, cursor, sizeof(msg.executed_shares));
    msg.executed_shares = be32toh(msg.executed_shares);
    cursor += sizeof(msg.executed_shares);

    std::memcpy(&msg.match_number, cursor, sizeof(msg.match_number));
    msg.match_number = be64toh(msg.match_number);

    return msg;
};

OrderExecutedPriceMessage parser::parse_order_executed_price(const std::byte* cursor) {
    OrderExecutedPriceMessage msg;

    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.order_reference_number, cursor, sizeof(msg.order_reference_number));
    msg.order_reference_number = be64toh(msg.order_reference_number);
    cursor += sizeof(msg.order_reference_number);

    std::memcpy(&msg.executed_shares, cursor, sizeof(msg.executed_shares));
    msg.executed_shares = be32toh(msg.executed_shares);
    cursor += sizeof(msg.executed_shares);

    std::memcpy(&msg.match_number, cursor, sizeof(msg.match_number));
    msg.match_number = be64toh(msg.match_number);

    std::memcpy(&msg.printable, cursor, sizeof(msg.printable));
    cursor += sizeof(msg.printable);

    std::memcpy(&msg.execution_price, cursor, sizeof(msg.execution_price));
    msg.execution_price = be32toh(msg.execution_price);

    return msg;
};

OrderCancelMessage parser::parse_order_cancel(const std::byte* cursor) {
    OrderCancelMessage msg;

    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.order_reference_number, cursor, sizeof(msg.order_reference_number));
    msg.order_reference_number = be64toh(msg.order_reference_number);
    cursor += sizeof(msg.order_reference_number);

    std::memcpy(&msg.cancelled_shares, cursor, sizeof(msg.cancelled_shares));
    msg.cancelled_shares = be32toh(msg.cancelled_shares);

    return msg;
};

OrderDeleteMessage parser::parse_order_delete(const std::byte* cursor) {
    OrderDeleteMessage msg;

    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.order_reference_number, cursor, sizeof(msg.order_reference_number));
    msg.order_reference_number = be64toh(msg.order_reference_number);
    cursor += sizeof(msg.order_reference_number);

    return msg;
};

OrderReplaceMessage parser::parse_order_replace(const std::byte* cursor) {
    OrderReplaceMessage msg;
    
    std::memcpy(&msg.stock_locate, cursor, sizeof(msg.stock_locate));
    msg.stock_locate = be16toh(msg.stock_locate);
    cursor += sizeof(msg.stock_locate);

    std::memcpy(&msg.tracking_number, cursor, sizeof(msg.tracking_number));
    msg.tracking_number = be16toh(msg.tracking_number);
    cursor += sizeof(msg.tracking_number);

    std::memcpy(msg.timestamp.data(), cursor, sizeof(msg.timestamp));
    cursor += sizeof(msg.timestamp);

    std::memcpy(&msg.original_order_reference_number, cursor, sizeof(msg.original_order_reference_number));
    msg.original_order_reference_number = be64toh(msg.original_order_reference_number);
    cursor += sizeof(msg.original_order_reference_number);

    std::memcpy(&msg.new_order_reference_number, cursor, sizeof(msg.new_order_reference_number));
    msg.new_order_reference_number = be64toh(msg.new_order_reference_number);
    cursor += sizeof(msg.new_order_reference_number);

    std::memcpy(&msg.shares, cursor, sizeof(msg.shares));
    msg.shares = be32toh(msg.shares);
    cursor += sizeof(msg.shares);

    std::memcpy(&msg.price, cursor, sizeof(msg.price));
    msg.price = be32toh(msg.price);

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
            break;
        buf_end += bytes_read;

        // Stop once the cursor is within the maximum message length + bytes for the length
        // We lose out on at most 2-3 messages per iteration through the buffer using this lazier stopping criterion
        while (cursor + MAX_MSG_SIZE + LENGTH_BYTES <= buf_end)
        {
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
                default:
                    break;
            }
            ++counter;
            cursor += len - 1;  // Move cursor to first byte of next message
        }
    }
};

