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