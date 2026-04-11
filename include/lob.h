#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <unordered_map>

#include <ankerl/unordered_dense.h>

#include "types.h"

using Ticker = std::array<char, 8>;
using Timestamp = std::array<uint8_t, 6>;

uint64_t ticker_key(const Ticker& t);

struct Order {
    uint32_t price;
    uint32_t shares;
    Side side;
    Timestamp timestamp;
    Ticker stock; // needed to find the right book
};

bool operator==(const Order& order1, const Order& order2);

std::ostream& operator<<(std::ostream& os, const Order& order);

// Orderbook is purely performs reconstruction
// Relies on data source to maintain correct orderbook invariants
class Orderbook {
public:
    std::unordered_map<uint32_t, uint64_t> bids;
    std::unordered_map<uint32_t, uint64_t> asks;

public:
    Orderbook() = default;

    Orderbook(const Orderbook&) = delete;
    Orderbook& operator=(const Orderbook&) = delete;

    ~Orderbook() = default;

    void addOrder(uint32_t price, uint32_t shares, Side side);
    void removeOrder(uint32_t price, uint32_t shares, Side side);
};

// Stores orderbooks for each ticker and all orders
class OrderbookManager {
protected:
    void handle(const AddOrderMessage& msg);
    void handle(const AddOrderMPIDAttributionMessage& msg);
    void handle(const OrderExecutedMessage& msg);
    void handle(const OrderExecutedPriceMessage& msg);
    void handle(const OrderCancelMessage& msg);
    void handle(const OrderDeleteMessage& msg);
    void handle(const OrderReplaceMessage& msg);
    void handle(const std::monostate& msg) {};

public:
    OrderbookManager() = default;

    OrderbookManager(const OrderbookManager&) = delete;
    OrderbookManager& operator=(const OrderbookManager&) = delete;

    void process(const Message& msg);

    // Consider to protected and implement setter/getter methods for encapsulation
    std::unordered_map<uint64_t, Orderbook> books;
    ankerl::unordered_dense::map<uint64_t, Order> orders;
};