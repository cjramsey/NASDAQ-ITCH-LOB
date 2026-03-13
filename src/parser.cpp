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

void ITCHReader::read_messages(std::function<void(const Message& msg)> process) {
    std::array<std::byte, BUFFER_SIZE> buffer;
    std::byte* cursor = buffer.data();
    std::byte* buf_end = buffer.data();
    
    while (true)
    {
        std::size_t leftover = buf_end - cursor;
        std::memmove(buffer.data(), cursor, leftover);
        cursor = buffer.data();
        buf_end = buffer.data() + leftover;

        file.read(reinterpret_cast<char*>(buf_end), BUFFER_SIZE - leftover);
        std::size_t bytes_read = file.gcount();
        if (bytes_read == 0) 
            break;
        buf_end += bytes_read;

        while (buf_end >= MAX_MSG_SIZE + cursor + 2)
        {
            uint16_t len_be;
            std::memcpy(&len_be, cursor, 2);
            uint16_t len = be16toh(len_be);

            uint8_t type = std::to_integer<uint8_t>(cursor[2]);
            
            switch (type)
            {
                case MessageType::AddOrder:
                {
                    AddOrderMessage msg = parser::parse_add_order(cursor + 3);
                    process(msg);
                    break;
                }
                case MessageType::AddOrderMPIDAttribution:
                {
                    AddOrderMPIDAttributionMessage msg = parser::parse_add_order_mpid(cursor + 3);
                    process(msg);
                    break;
                }
                case MessageType::OrderExecuted:
                {
                    OrderExecutedMessage msg = parser::parse_order_executed(cursor + 3);
                    process(msg);
                    break;
                }
                case MessageType::OrderExecutedPrice:
                {
                    OrderExecutedPriceMessage msg = parser::parse_order_executed_price(cursor + 3);
                    process(msg);
                    break;
                }
                case MessageType::OrderCancel:
                {
                    OrderCancelMessage msg = parser::parse_order_cancel(cursor + 3);
                    process(msg);
                    break;
                }
                case MessageType::OrderDelete:
                {
                    OrderDeleteMessage msg = parser::parse_order_delete(cursor + 3);
                    process(msg);
                    break;
                }
                case MessageType::OrderReplace:
                {
                    OrderReplaceMessage msg = parser::parse_order_replace(cursor + 3);
                    process(msg);
                    break;
                }
                default:
                    break;
            }

            cursor += 2 + len;
        }
    }
};

