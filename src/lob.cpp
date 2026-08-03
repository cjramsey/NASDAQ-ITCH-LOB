#include <cassert>
#include <cstring>
#include <cstdint>
#include <variant>
#include "lob.h"

bool operator==(const Order& order1, const Order& order2) {
    return (
        order1.price == order2.price &&
        order1.shares == order2.shares &&
        order1.side == order2.side &&
        order1.timestamp == order2.timestamp &&
        order1.stock == order2.stock
    );
}

std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << "Side: " << ((order.side == Side::Buy) ? "Buy" : "Sell") << '\n';
    os << "Price = " << order.price << '\n';
    os << "Shares = " << order.shares << '\n';
    os << " Stock = ";
    for (const auto& c: order.stock)
        os << c;
    os << '\n';
    os << " Timestamp: " << parse_timestamp(order.timestamp);
    return os;
}

// For using ticker as key when accessing relevant orderbook
uint64_t ticker_key(const Ticker& t) {
    uint64_t key = 0;
    std::memcpy(&key, t.data(), 8); // Converts bytes directly to 64-bit unsigned integer
    return key;
}

void HashMapBook::add(uint32_t price, uint32_t shares) {
    levels[price] += shares;
}

void HashMapBook::remove(uint32_t price, uint32_t shares) {
    auto it{levels.find(price)};
    assert(it != levels.end() && "removeOrder referenced a price level with no resting quantity");

    it->second -= shares;
    if (it->second == 0)
        levels.erase(it);
}

bool HashMapBook::contains(uint32_t price) const {
    return levels.contains(price);
}

uint64_t HashMapBook::sharesAt(uint32_t price) const {
    auto it{levels.find(price)};
    assert(it != levels.end() && "sharesAt referenced a price level with no resting quantity");
    return it->second;
}

template <typename Compare>
std::vector<PriceLevel>::iterator SortedVectorBook<Compare>::find(uint32_t price) {
    return std::lower_bound(levels.begin(), levels.end(), price,
        [](const PriceLevel& level, uint32_t p) { return Compare{}(level.price, p); });
}

template <typename Compare>
std::vector<PriceLevel>::const_iterator SortedVectorBook<Compare>::find(uint32_t price) const {
    return std::lower_bound(levels.begin(), levels.end(), price,
        [](const PriceLevel& level, uint32_t p) { return Compare{}(level.price, p); });
}

template <typename Compare>
void SortedVectorBook<Compare>::add(uint32_t price, uint32_t shares) {
    auto it{find(price)};
    if (it != levels.end() && it->price == price)
        it->shares += shares;
    else
        levels.insert(it, PriceLevel{price, shares});
}

template <typename Compare>
void SortedVectorBook<Compare>::remove(uint32_t price, uint32_t shares) {
    auto it{find(price)};
    assert(it != levels.end() && it->price == price && "removeOrder referenced a price level with no resting quantity");

    it->shares -= shares;
    if (it->shares == 0)
        levels.erase(it);
}

template <typename Compare>
bool SortedVectorBook<Compare>::contains(uint32_t price) const {
    auto it{find(price)};
    return it != levels.end() && it->price == price;
}

template <typename Compare>
uint64_t SortedVectorBook<Compare>::sharesAt(uint32_t price) const {
    auto it{find(price)};
    assert(it != levels.end() && it->price == price && "sharesAt referenced a price level with no resting quantity");
    return it->shares;
}

template <typename Compare>
uint32_t SortedVectorBook<Compare>::best() const {
    assert(!levels.empty() && "best referenced an empty book");
    return levels.front().price;
}

template <typename BidBook, typename AskBook>
void OrderbookT<BidBook, AskBook>::addOrder(uint32_t price, uint32_t shares, Side side) {
    if (side == Side::Buy)
        bids.add(price, shares);
    else
        asks.add(price, shares);
}

template <typename BidBook, typename AskBook>
void OrderbookT<BidBook, AskBook>::removeOrder(uint32_t price, uint32_t shares, Side side) {
    if (side == Side::Buy)
        bids.remove(price, shares);
    else
        asks.remove(price, shares);
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::process(const Message& msg) {
    std::visit([this](const auto& m) {
        handle(m);
    }, msg);
}

// handle method overloaded for each parsed message type
// Delegates orderbook logic to correct orderbook and performs housekeeping for orders

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const AddOrderMessage& msg) {
    // Defer price level logic to orderbook itself
    books[ticker_key(msg.stock)].addOrder(msg.price, msg.shares, msg.side);

    orders[msg.order_reference_number] = {msg.price, msg.shares, msg.side, msg.timestamp, msg.stock};
}

// ignore market participant id for now
template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const AddOrderMPIDAttributionMessage& msg) {
    books[ticker_key(msg.stock)].addOrder(msg.price, msg.shares, msg.side);

    orders[msg.order_reference_number] = {msg.price, msg.shares, msg.side, msg.timestamp, msg.stock};
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const OrderDeleteMessage& msg) {
    auto it{orders.find(msg.order_reference_number)};
    assert(it != orders.end() && "OrderDelete referenced an unknown order_reference_number");

    Order order{it->second};

    books[ticker_key(order.stock)].removeOrder(order.price, order.shares, order.side);

    orders.erase(it);
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const OrderCancelMessage& msg) {
    auto it{orders.find(msg.order_reference_number)};
    assert(it != orders.end() && "OrderCancel referenced an unknown order_reference_number");

    Order& order{it->second};   // use reference to minimize hashes performed, compiler might elide anyway

    order.shares -= msg.cancelled_shares;

    books[ticker_key(order.stock)].removeOrder(order.price, msg.cancelled_shares, order.side);
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const OrderReplaceMessage& msg) {
    auto it{orders.find(msg.original_order_reference_number)};
    assert(it != orders.end() && "OrderReplace referenced an unknown original_order_reference_number");

    Order order{it->second};

    uint64_t key{ ticker_key(order.stock) };

    books[key].removeOrder(order.price, order.shares, order.side);

    books[key].addOrder(msg.price, msg.shares, order.side);

    orders[msg.new_order_reference_number] = {msg.price, msg.shares, order.side, msg.timestamp, order.stock};

    orders.erase(msg.original_order_reference_number);
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const OrderExecutedMessage& msg) {
    // Using nests scope because inserting or removing keys invalidates references
    uint32_t shares_remaining;
    {
        auto it{orders.find(msg.order_reference_number)};
        assert(it != orders.end() && "OrderExecuted referenced an unknown order_reference_number");

        Order& order{it->second};

        books[ticker_key(order.stock)].removeOrder(order.price, msg.executed_shares, order.side);

        order.shares -= msg.executed_shares;
        shares_remaining = order.shares;
    }

    if (shares_remaining == 0)
    {
        orders.erase(msg.order_reference_number);
    }
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::handle(const OrderExecutedPriceMessage& msg) {
    uint32_t shares_remaining;
    {
        auto it{orders.find(msg.order_reference_number)};
        assert(it != orders.end() && "OrderExecutedPrice referenced an unknown order_reference_number");

        Order& order{it->second};

        books[ticker_key(order.stock)].removeOrder(order.price, msg.executed_shares, order.side);

        order.shares -= msg.executed_shares;
        shares_remaining = order.shares;
    }

    if (shares_remaining == 0)
    {
        orders.erase(msg.order_reference_number);
    }
}

template <typename OrderbookT_>
void OrderbookManager<OrderbookT_>::reset() {
    orders.clear();
    books.clear();
}

// Explicit instantiation: generates code once, here, for exactly the backends we support
template class SortedVectorBook<std::greater<uint32_t>>;
template class SortedVectorBook<std::less<uint32_t>>;

template class OrderbookT<HashMapBook, HashMapBook>;
template class OrderbookT<SortedVectorBook<std::greater<uint32_t>>, SortedVectorBook<std::less<uint32_t>>>;

template class OrderbookManager<FastOrderbook>;
template class OrderbookManager<BBOOrderbook>;
