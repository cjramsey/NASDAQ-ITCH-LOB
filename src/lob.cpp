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
        order2.stock == order2.stock
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

void OrderbookManager::process(const Message& msg) {
    std::visit([this](const auto& m) {
        handle(m);
    }, msg);
}

// handle method overloaded for each parsed message type
// Delegates orderbook logic to correct orderbook and performs housekeeping for orders 

void OrderbookManager::handle(const AddOrderMessage& msg) {
    // Defer price level logic to orderbook itself
    books[ticker_key(msg.stock)].addOrder(msg.price, msg.shares, msg.side);

    orders[msg.order_reference_number] = {msg.price, msg.shares, msg.side, msg.timestamp, msg.stock};
}

// ignore market participant id for now
void OrderbookManager::handle(const AddOrderMPIDAttributionMessage& msg) {
    books[ticker_key(msg.stock)].addOrder(msg.price, msg.shares, msg.side);

    orders[msg.order_reference_number] = {msg.price, msg.shares, msg.side, msg.timestamp, msg.stock};
}

void OrderbookManager::handle(const OrderDeleteMessage& msg) {
    Order order{orders[msg.order_reference_number]};

    uint32_t price{ order.price };
    uint32_t shares{ order.shares };
    Side side{ order.side };

    books[ticker_key(order.stock)].removeOrder(price, shares, side);

    orders.erase(msg.order_reference_number);
}

void OrderbookManager::handle(const OrderCancelMessage& msg) {
    Order& order{orders[msg.order_reference_number]};   // use reference to minimize hashes performed, compiler might elide anyway

    order.shares -= msg.cancelled_shares;

    books[ticker_key(order.stock)].removeOrder(order.price, msg.cancelled_shares, order.side);
}

void OrderbookManager::handle(const OrderReplaceMessage& msg) {
    Order order{orders[msg.original_order_reference_number]};

    uint64_t key{ ticker_key(order.stock) };

    books[key].removeOrder(order.price, order.shares, order.side);

    books[key].addOrder(msg.price, msg.shares, order.side);

    orders[msg.new_order_reference_number] = {msg.price, msg.shares, order.side, msg.timestamp, order.stock};

    orders.erase(msg.original_order_reference_number);
}

void OrderbookManager::handle(const OrderExecutedMessage& msg) {
    // Using nests scope because inserting or removing keys invalidates references
    uint32_t shares_remaining;
    {
        Order& order{orders[msg.order_reference_number]};

        books[ticker_key(order.stock)].removeOrder(order.price, msg.executed_shares, order.side);

        order.shares -= msg.executed_shares;
        shares_remaining = order.shares;
    }

    if (shares_remaining == 0)
    {
        orders.erase(msg.order_reference_number);
    }
}

void OrderbookManager::handle(const OrderExecutedPriceMessage& msg) {
    uint32_t shares_remaining;
    {
        Order& order{orders[msg.order_reference_number]};

        books[ticker_key(order.stock)].removeOrder(order.price, msg.executed_shares, order.side);

        order.shares -= msg.executed_shares;
        shares_remaining = order.shares;
    }

    if (shares_remaining == 0)
    {
        orders.erase(msg.order_reference_number);
    }
}

void Orderbook::addOrder(uint32_t price, uint32_t shares, Side side) {
    if (side == Side::Buy)
    {
        bids[price] += shares;
    }
    else
    {
        asks[price] += shares;
    }
}

void Orderbook::removeOrder(uint32_t price, uint32_t shares, Side side) {
    if (side == Side::Buy)
    {
        bids[price] -= shares;
        if (bids[price] == 0)
            bids.erase(price);
    }
    else
    {
        asks[price] -= shares;
        if (asks[price] == 0)
            asks.erase(price);
    }
}