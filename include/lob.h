#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <vector>

#include <ankerl/unordered_dense.h>

#include "types.h"

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

struct PriceLevel {
    uint32_t price;
    uint64_t shares;
};

// Aggregated shares per price, keyed by price. O(1) avg add/remove.
// BBO requires an O(n) scan since entries are unordered.
class HashMapBook {
public:
    void add(uint32_t price, uint32_t shares);
    void remove(uint32_t price, uint32_t shares);
    bool contains(uint32_t price) const;
    uint64_t sharesAt(uint32_t price) const;

private:
    std::unordered_map<uint32_t, uint64_t> levels;
};

// Aggregated shares per price, kept sorted by Compare. O(log n) add/remove,
// O(1) BBO (front of the vector).
template <typename Compare>
class SortedVectorBook {
public:
    void add(uint32_t price, uint32_t shares);
    void remove(uint32_t price, uint32_t shares);
    bool contains(uint32_t price) const;
    uint64_t sharesAt(uint32_t price) const;
    uint32_t best() const;

private:
    std::vector<PriceLevel>::iterator find(uint32_t price);
    std::vector<PriceLevel>::const_iterator find(uint32_t price) const;

    std::vector<PriceLevel> levels;
};

extern template class SortedVectorBook<std::greater<uint32_t>>;
extern template class SortedVectorBook<std::less<uint32_t>>;

// Orderbook is purely performs reconstruction
// Relies on data source to maintain correct orderbook invariants
template <typename BidBook, typename AskBook>
class OrderbookT {
public:
    BidBook bids;
    AskBook asks;

public:
    OrderbookT() = default;

    OrderbookT(const OrderbookT&) = delete;
    OrderbookT& operator=(const OrderbookT&) = delete;

    ~OrderbookT() = default;

    void addOrder(uint32_t price, uint32_t shares, Side side);
    void removeOrder(uint32_t price, uint32_t shares, Side side);
};

// Fast reconstruction, no ordering maintained (BBO would require an O(n) scan)
using FastOrderbook = OrderbookT<HashMapBook, HashMapBook>;
// O(1) BBO, at the cost of O(log n) updates and O(n) level open/close
using BBOOrderbook = OrderbookT<SortedVectorBook<std::greater<uint32_t>>, SortedVectorBook<std::less<uint32_t>>>;

extern template class OrderbookT<HashMapBook, HashMapBook>;
extern template class OrderbookT<SortedVectorBook<std::greater<uint32_t>>, SortedVectorBook<std::less<uint32_t>>>;

// Stores orderbooks for each ticker and all orders
template <typename OrderbookT_>
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

    void reset();

    // Consider to protected and implement setter/getter methods for encapsulation
    std::unordered_map<uint64_t, OrderbookT_> books;
    ankerl::unordered_dense::map<uint64_t, Order> orders;
};

extern template class OrderbookManager<FastOrderbook>;
extern template class OrderbookManager<BBOOrderbook>;
