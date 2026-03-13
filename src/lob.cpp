#include <cstring>
#include <cstdint>
#include <variant>
#include "lob.h"

std::ostream& operator<<(std::ostream& os, const Order& order) {
    os << ((order.side == Side::Buy) ? "Buy: " : "Sell: ");
    os << "Price = " << order.price << ' ' << "Shares = " << order.shares;
    os << " Stock = ";
    for (const auto& c: order.stock)
        os << c;
    os << " Timestamp: " << parse_timestamp(order.timestamp);
    os << '\n';
    return os;
}

uint64_t ticker_key(const Ticker& t) {
    uint64_t key;
    std::memcpy(&key, t.data(), 8);
    return key;
}

void OrderbookManager::process(const Message& msg) {
    std::visit([this](const auto& m) {
        handle(m);
    }, msg);
}

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
    Order order{orders[msg.order_reference_number]};

    order.shares -= msg.cancelled_shares;

    books[ticker_key(order.stock)].removeOrder(order.price, msg.cancelled_shares, order.side);
}

void OrderbookManager::handle(const OrderReplaceMessage& msg) {
    Order order{orders[msg.original_order_reference_number]};

    uint64_t key{ ticker_key(order.stock) };

    books[key].removeOrder(order.price, order.shares, order.side);

    orders.erase(msg.original_order_reference_number);

    books[key].addOrder(msg.price, msg.shares, order.side);

    orders[msg.new_order_reference_number] = {msg.price, msg.shares, order.side, msg.timestamp, order.stock};
}

void OrderbookManager::handle(const OrderExecutedMessage& msg) {
    Order order{orders[msg.order_reference_number]};

    books[ticker_key(order.stock)].removeOrder(order.price, order.shares, order.side);

    orders[msg.order_reference_number].shares -= msg.executed_shares;

    if (orders[msg.order_reference_number].shares == 0)
    {
        orders.erase(msg.order_reference_number);
    }
}

void OrderbookManager::handle(const OrderExecutedPriceMessage& msg) {
    Order order{orders[msg.order_reference_number]};

    books[ticker_key(order.stock)].removeOrder(order.price, order.shares, order.side);

    orders[msg.order_reference_number].shares -= msg.executed_shares;

    if (orders[msg.order_reference_number].shares == 0)
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
        asks[price] += shares;
        if (asks[price] == 0)
            asks.erase(price);
    }
}