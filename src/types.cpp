#include <cstdint>
#include <iostream>
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

namespace {
    // Prints whichever of these fields T actually has, in a fixed order,
    // so every message type gets a consistent operator<< with no duplication.
    template <typename T>
    std::ostream& print_message(std::ostream& os, const T& msg) {
        os << "Stock Locate: " << msg.stock_locate << '\n';
        os << "Tracking Number: " << msg.tracking_number << '\n';
        os << "Timestamp: " << parse_timestamp(msg.timestamp) << '\n';

        if constexpr (requires { msg.order_reference_number; })
            os << "Order Ref. No.: " << msg.order_reference_number << '\n';
        if constexpr (requires { msg.original_order_reference_number; })
            os << "Original Order Ref. No.: " << msg.original_order_reference_number << '\n';
        if constexpr (requires { msg.new_order_reference_number; })
            os << "New Order Ref. No.: " << msg.new_order_reference_number << '\n';
        if constexpr (requires { msg.side; })
            os << "Side: " << (msg.side == Side::Sell ? "sell" : "buy") << '\n';
        if constexpr (requires { msg.shares; })
            os << "Shares: " << msg.shares << '\n';
        if constexpr (requires { msg.executed_shares; })
            os << "Executed Shares: " << msg.executed_shares << '\n';
        if constexpr (requires { msg.cancelled_shares; })
            os << "Cancelled Shares: " << msg.cancelled_shares << '\n';
        if constexpr (requires { msg.stock; })
        {
            os << "Stock: ";
            for (const auto& c: msg.stock)
                os << c;
            os << '\n';
        }
        if constexpr (requires { msg.price; })
            os << "Price: " << msg.price << '\n';
        if constexpr (requires { msg.execution_price; })
            os << "Execution Price: " << msg.execution_price << '\n';
        if constexpr (requires { msg.match_number; })
            os << "Match Number: " << msg.match_number << '\n';
        if constexpr (requires { msg.printable; })
            os << "Printable: " << msg.printable << '\n';
        if constexpr (requires { msg.MPID; })
            os << "MPID: " << msg.MPID << '\n';

        return os;
    }
}

std::ostream& operator<<(std::ostream& os, const AddOrderMessage& msg) {
    return print_message(os, msg);
}

std::ostream& operator<<(std::ostream& os, const AddOrderMPIDAttributionMessage& msg) {
    return print_message(os, msg);
}

std::ostream& operator<<(std::ostream& os, const OrderExecutedMessage& msg) {
    return print_message(os, msg);
}

std::ostream& operator<<(std::ostream& os, const OrderExecutedPriceMessage& msg) {
    return print_message(os, msg);
}

std::ostream& operator<<(std::ostream& os, const OrderCancelMessage& msg) {
    return print_message(os, msg);
}

std::ostream& operator<<(std::ostream& os, const OrderDeleteMessage& msg) {
    return print_message(os, msg);
}

std::ostream& operator<<(std::ostream& os, const OrderReplaceMessage& msg) {
    return print_message(os, msg);
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