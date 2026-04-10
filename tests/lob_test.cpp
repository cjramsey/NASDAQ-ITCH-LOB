#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <types.h>
#include <lob.h>


class OrderbookTest : public ::testing::Test {
protected:
    Orderbook orderbook;

    void SetUp() override {
        orderbook.addOrder(100, 10, Side::Buy);
        orderbook.addOrder(200, 10, Side::Buy);

        orderbook.addOrder(90, 10, Side::Sell);
        orderbook.addOrder(50, 10, Side::Sell);
    }
};

TEST_F(OrderbookTest, AddSingleOrder) {
    orderbook.addOrder(10000, 10, Side::Buy);
    EXPECT_EQ(orderbook.bids[10000], 10);

    orderbook.addOrder(10000, 10, Side::Sell);
    EXPECT_EQ(orderbook.asks[10000], 10);
}

TEST_F(OrderbookTest, AddTwoOrdersSameLevel) {
    orderbook.addOrder(10001, 20, Side::Buy);
    orderbook.addOrder(10001, 5, Side::Buy);
    EXPECT_EQ(orderbook.bids[10001], 25);

    orderbook.addOrder(9999, 15, Side::Sell);
    orderbook.addOrder(9999, 25, Side::Sell);    
    EXPECT_EQ(orderbook.asks[9999], 40);
}

TEST_F(OrderbookTest, RemoveOrderLevelStillExists) {
    orderbook.addOrder(50000, 10, Side::Buy);
    orderbook.removeOrder(50000, 5, Side::Buy);
    EXPECT_EQ(orderbook.bids[50000], 5);

    orderbook.addOrder(50000, 10, Side::Sell);
    orderbook.removeOrder(50000, 5, Side::Sell);
    EXPECT_EQ(orderbook.asks[50000], 5);
}

TEST_F(OrderbookTest, RemoveOrderLevelRemoved) {
    orderbook.removeOrder(100, 10, Side::Buy);
    EXPECT_FALSE(orderbook.bids.contains(100));

    orderbook.removeOrder(90, 10, Side::Sell);
    EXPECT_FALSE(orderbook.bids.contains(90));
}


Ticker convertStringToTicker(const std::string& name) {
    Ticker ticker{};
    std::copy(name.begin(), 
              name.begin() + std::min(name.length(), ticker.size()),
              ticker.begin());
    return ticker;
} 

TEST(TickerTest, ConvertStringTicker) {
    Ticker test_ticker{'A', 'A', 'P', 'L'};
    EXPECT_EQ(convertStringToTicker("AAPL"), test_ticker);
}

TEST(TickerTest, ConvertStringTooLong) {
    Ticker test_ticker{'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    EXPECT_EQ(convertStringToTicker("ABCDEFGHI"), test_ticker);
}

TEST(TickerTest, TickerKeyFunction) {
    Ticker test_ticker{'A', 'A', 'P', 'L'};
    EXPECT_EQ(ticker_key(test_ticker), 0x000000004C504141); // little-endian byte-order
}


Timestamp convertIntegerToTimestamp(const uint64_t value) {
    Timestamp time{};
    std::memcpy(time.data(), &value, std::min(sizeof(value), sizeof(time)));
    return time;
}

TEST(TickerTest, ConvertIntToTimeStamp) {
    Timestamp time{0, 0, 0, 0, 255, 255};
    EXPECT_EQ(convertIntegerToTimestamp(0xFFFF00000000), time);
}


class OrderBookManagerTest : public ::testing::Test  {
protected:
    struct TestableManager : public OrderbookManager {
        using OrderbookManager::handle;
    };

    TestableManager orderbook_manager;

    void SetUp() override {
        AddOrderMessage bid{.stock_locate=0,
                            .tracking_number=0,
                            .timestamp=convertIntegerToTimestamp(0),
                            .order_reference_number=1,
                            .side=Side::Buy,
                            .shares=10,
                            .stock=convertStringToTicker("AAPL"),
                            .price=1000
        };
        AddOrderMessage ask{.stock_locate=0,
                            .tracking_number=0,
                            .timestamp=convertIntegerToTimestamp(1),
                            .order_reference_number=2,
                            .side=Side::Sell,
                            .shares=10,
                            .stock=convertStringToTicker("SPY"),
                            .price=1000
        };
        orderbook_manager.handle(bid);
        orderbook_manager.handle(ask);
    }
};

TEST_F(OrderBookManagerTest, HandleAddOrderMessageBid) {
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_EQ(book.bids[1000], 10);

    Order expected{.price=1000, 
                   .shares=10, 
                   .side=Side::Buy, 
                   .timestamp=convertIntegerToTimestamp(0), 
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(orderbook_manager.orders[1], expected);
}

TEST_F(OrderBookManagerTest, HandleAddOrderMessageAsk) {
    uint64_t key = ticker_key(convertStringToTicker("SPY"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_EQ(book.asks[1000], 10);

    Order expected{.price=1000, 
                   .shares=10, 
                   .side=Side::Sell, 
                   .timestamp=convertIntegerToTimestamp(1), 
                   .stock=convertStringToTicker("SPY")
    };
    EXPECT_EQ(orderbook_manager.orders[2], expected);
}

TEST_F(OrderBookManagerTest, HandlePartialOrderExecutedBid) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .executed_shares=5,
        .match_number=0
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_EQ(book.bids[1000], 5);

    Order expected{.price=1000, 
                   .shares=5, 
                   .side=Side::Buy, 
                   .timestamp=convertIntegerToTimestamp(0), 
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(orderbook_manager.orders[1], expected);
}

TEST_F(OrderBookManagerTest, HandlePartialOrderExecutedAsk) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=2,
        .executed_shares=5,
        .match_number=0
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("SPY"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_EQ(book.asks[1000], 5);

    Order expected{.price=1000, 
                   .shares=5, 
                   .side=Side::Sell, 
                   .timestamp=convertIntegerToTimestamp(1), 
                   .stock=convertStringToTicker("SPY")
    };
    EXPECT_EQ(orderbook_manager.orders[2], expected);
}

TEST_F(OrderBookManagerTest, HandleFillOrderExecutedBid) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .executed_shares=10,
        .match_number=0
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));

    EXPECT_FALSE(orderbook_manager.orders.contains(1));
}

TEST_F(OrderBookManagerTest, HandleFillOrderExecutedAsk) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=2,
        .executed_shares=10,
        .match_number=0
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("SPY"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_FALSE(book.asks.contains(1000));

    EXPECT_FALSE(orderbook_manager.orders.contains(2));
}

TEST_F(OrderBookManagerTest, HandleOrderExecutedPrice) {
    OrderExecutedPriceMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .executed_shares=10,
        .match_number=0,
        .printable='Y',
        .execution_price=1000
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));

    EXPECT_FALSE(orderbook_manager.orders.contains(1));
}

TEST_F(OrderBookManagerTest, HandleOrderCancel) {
    OrderCancelMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .cancelled_shares=5
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_EQ(book.bids[1000], 5);

    Order expected{.price=1000, 
                   .shares=5, 
                   .side=Side::Buy, 
                   .timestamp=convertIntegerToTimestamp(0), 
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(orderbook_manager.orders[1], expected);
}

TEST_F(OrderBookManagerTest, HandleOrderReplace) {
    OrderReplaceMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .original_order_reference_number=1,
        .new_order_reference_number=3,
        .shares=30,
        .price=1010
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));
    EXPECT_EQ(book.bids[1010], 30);

    Order expected{.price=1010, 
                   .shares=30, 
                   .side=Side::Buy, 
                   .timestamp=convertIntegerToTimestamp(2), 
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(orderbook_manager.orders[3], expected);
    EXPECT_FALSE(orderbook_manager.orders.contains(1));
}

TEST_F(OrderBookManagerTest, HandleOrderDelete) {
    OrderDeleteMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1
    };
    orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    Orderbook& book = orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));
    EXPECT_FALSE(orderbook_manager.orders.contains(1));
}