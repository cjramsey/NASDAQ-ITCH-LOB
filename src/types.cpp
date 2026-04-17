#include <cstdint>
#include <iostream>
#include <string>
#include <types.h>

uint64_t parse_timestamp(const std::array<uint8_t, 6>& timestamp) {
    uint64_t t{};
    for (const auto& i: timestamp)
    {
        t <<= 8;
        t += i;
    }
    return t;
};

std::string parse_stock(const std::array<char, 8>& stock) {
    std::string s{};
    for (const auto& c: stock)
    {
        if (!std::isspace(c))
        {
            s += c;
        }
        else break;
    }
    return s;
}

std::ostream& operator<<(std::ostream& os, const AddOrderMessage& msg)
{
    os << "Stock Locate: " << msg.stock_locate << std::endl;
    os << "Tracking Number: " << msg.tracking_number << std::endl;
    os << "Timestamp: " << parse_timestamp(msg.timestamp) << std::endl;
    os << "Order Ref. No.: " << msg.order_reference_number << std::endl;
    os << "side: " << (msg.side == Side::Sell ? "sell" : "buy") << std::endl;
    os << "Shares: " << msg.shares << std::endl;
    os << "Stock: ";
    for (const auto& c: msg.stock)
    {
        os << c;
    }
    os << '\n';
    os << "Price: " << msg.price << std::endl;
    return os;
}

bool operator==(const AddOrderMessage& lhs, const AddOrderMessage& rhs) {
    return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.order_reference_number == rhs.order_reference_number &&
        lhs.side == rhs.side &&
        lhs.shares == rhs.shares &&
        lhs.stock == rhs.stock &&
        lhs.price == rhs.price
    );
}

bool operator==(const AddOrderMPIDAttributionMessage& lhs, const AddOrderMPIDAttributionMessage& rhs) {
    return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.order_reference_number == rhs.order_reference_number &&
        lhs.side == rhs.side &&
        lhs.shares == rhs.shares &&
        lhs.stock == rhs.stock &&
        lhs.price == rhs.price &&
        lhs.MPID == rhs.MPID
    );
}

bool operator==(const OrderExecutedMessage& lhs, const OrderExecutedMessage& rhs) {
    return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.order_reference_number == rhs.order_reference_number &&
        lhs.executed_shares == rhs.executed_shares &&
        lhs.match_number == rhs.match_number
    );
}

bool operator==(const OrderExecutedPriceMessage& lhs, const OrderExecutedPriceMessage& rhs) {
     return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.order_reference_number == rhs.order_reference_number &&
        lhs.executed_shares == rhs.executed_shares &&
        lhs.match_number == rhs.match_number &&
        lhs.printable == rhs.printable && 
        lhs.execution_price == rhs.execution_price
    );
}

bool operator==(const OrderCancelMessage& lhs, const OrderCancelMessage& rhs) {
     return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.order_reference_number == rhs.order_reference_number &&
        lhs.cancelled_shares == rhs.cancelled_shares
    );
}

bool operator==(const OrderDeleteMessage& lhs, const OrderDeleteMessage& rhs) {
     return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.order_reference_number == rhs.order_reference_number
    );
}

bool operator==(const OrderReplaceMessage& lhs, const OrderReplaceMessage& rhs) {
     return (
        lhs.stock_locate == rhs.stock_locate &&
        lhs.tracking_number == rhs.tracking_number &&
        lhs.timestamp == rhs.timestamp &&
        lhs.original_order_reference_number == rhs.original_order_reference_number &&
        lhs.new_order_reference_number == rhs.new_order_reference_number &&
        lhs.shares == rhs.shares && 
        lhs.price == rhs.price
    );
}