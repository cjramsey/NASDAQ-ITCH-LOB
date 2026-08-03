#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>
#include <sstream>
#include <types.h>
#include <parser.h>
#include <lob.h>

using OrderbookBackends = ::testing::Types<FastOrderbook, BBOOrderbook>;

template <typename T>
class OrderbookTest : public ::testing::Test {
protected:
    T orderbook;

    void SetUp() override {
        orderbook.addOrder(100, 10, Side::Buy);
        orderbook.addOrder(200, 10, Side::Buy);

        orderbook.addOrder(90, 10, Side::Sell);
        orderbook.addOrder(50, 10, Side::Sell);
    }
};
TYPED_TEST_SUITE(OrderbookTest, OrderbookBackends);

TYPED_TEST(OrderbookTest, AddSingleOrder) {
    this->orderbook.addOrder(10000, 10, Side::Buy);
    EXPECT_EQ(this->orderbook.bids.sharesAt(10000), 10);

    this->orderbook.addOrder(10000, 10, Side::Sell);
    EXPECT_EQ(this->orderbook.asks.sharesAt(10000), 10);
}

TYPED_TEST(OrderbookTest, AddTwoOrdersSameLevel) {
    this->orderbook.addOrder(10001, 20, Side::Buy);
    this->orderbook.addOrder(10001, 5, Side::Buy);
    EXPECT_EQ(this->orderbook.bids.sharesAt(10001), 25);

    this->orderbook.addOrder(9999, 15, Side::Sell);
    this->orderbook.addOrder(9999, 25, Side::Sell);
    EXPECT_EQ(this->orderbook.asks.sharesAt(9999), 40);
}

TYPED_TEST(OrderbookTest, RemoveOrderLevelStillExists) {
    this->orderbook.addOrder(50000, 10, Side::Buy);
    this->orderbook.removeOrder(50000, 5, Side::Buy);
    EXPECT_EQ(this->orderbook.bids.sharesAt(50000), 5);

    this->orderbook.addOrder(50000, 10, Side::Sell);
    this->orderbook.removeOrder(50000, 5, Side::Sell);
    EXPECT_EQ(this->orderbook.asks.sharesAt(50000), 5);
}

TYPED_TEST(OrderbookTest, RemoveOrderLevelRemoved) {
    this->orderbook.removeOrder(100, 10, Side::Buy);
    EXPECT_FALSE(this->orderbook.bids.contains(100));

    this->orderbook.removeOrder(90, 10, Side::Sell);
    EXPECT_FALSE(this->orderbook.asks.contains(90));
}


// best() only exists on the sorted-vector backend, so these are exercised
// directly against BBOOrderbook rather than as typed tests.

TEST(BBOOrderbookTest, BestAfterSingleAdd) {
    BBOOrderbook book;
    book.addOrder(100, 10, Side::Buy);
    EXPECT_EQ(book.bids.best(), 100);

    book.addOrder(200, 10, Side::Sell);
    EXPECT_EQ(book.asks.best(), 200);
}

TEST(BBOOrderbookTest, BestBidTracksHighestPrice) {
    BBOOrderbook book;
    book.addOrder(100, 10, Side::Buy);
    book.addOrder(105, 10, Side::Buy);
    book.addOrder(95, 10, Side::Buy);
    EXPECT_EQ(book.bids.best(), 105);
}

TEST(BBOOrderbookTest, BestAskTracksLowestPrice) {
    BBOOrderbook book;
    book.addOrder(200, 10, Side::Sell);
    book.addOrder(190, 10, Side::Sell);
    book.addOrder(210, 10, Side::Sell);
    EXPECT_EQ(book.asks.best(), 190);
}

TEST(BBOOrderbookTest, BestFallsBackAfterLevelDrained) {
    BBOOrderbook book;
    book.addOrder(105, 10, Side::Buy);
    book.addOrder(100, 10, Side::Buy);
    ASSERT_EQ(book.bids.best(), 105);

    book.removeOrder(105, 10, Side::Buy); // fully drains the best level
    EXPECT_EQ(book.bids.best(), 100);
}

TEST(BBOOrderbookTest, BidAndAskTrackedIndependently) {
    BBOOrderbook book;
    book.addOrder(100, 10, Side::Buy);
    book.addOrder(100, 10, Side::Sell);
    EXPECT_EQ(book.bids.best(), 100);
    EXPECT_EQ(book.asks.best(), 100);

    book.addOrder(110, 10, Side::Buy);
    book.addOrder(90, 10, Side::Sell);
    EXPECT_EQ(book.bids.best(), 110);
    EXPECT_EQ(book.asks.best(), 90);
}

#ifndef NDEBUG
TEST(BBOOrderbookTest, BestOnEmptyBookAsserts) {
    BBOOrderbook book;
    EXPECT_DEATH(book.bids.best(), "");
}
#endif


TEST(OrderPrintTest, StreamsAllFields) {
    Order order{.price=1000,
                .shares=25,
                .side=Side::Sell,
                .timestamp=convertIntegerToTimestamp(123),
                .stock=convertStringToTicker("AAPL")
    };

    std::ostringstream os;
    os << order;
    std::string output = os.str();

    EXPECT_NE(output.find("Sell"), std::string::npos);
    EXPECT_NE(output.find("1000"), std::string::npos);
    EXPECT_NE(output.find("25"), std::string::npos);
    EXPECT_NE(output.find("AAPL"), std::string::npos);
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


TEST(TickerTest, ConvertIntToTimeStamp) {
    Timestamp time{0, 0, 0, 0, 255, 255};
    EXPECT_EQ(convertIntegerToTimestamp(0xFFFF00000000), time);
}


template <typename T>
class OrderBookManagerTest : public ::testing::Test  {
protected:
    struct TestableManager : public OrderbookManager<T> {
        using OrderbookManager<T>::handle;
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
        this->orderbook_manager.handle(bid);
        this->orderbook_manager.handle(ask);
    }
};
TYPED_TEST_SUITE(OrderBookManagerTest, OrderbookBackends);

TYPED_TEST(OrderBookManagerTest, HandleAddOrderMessageBid) {
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.bids.sharesAt(1000), 10);

    Order expected{.price=1000,
                   .shares=10,
                   .side=Side::Buy,
                   .timestamp=convertIntegerToTimestamp(0),
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(this->orderbook_manager.orders[1], expected);
}

TYPED_TEST(OrderBookManagerTest, HandleAddOrderMessageAsk) {
    uint64_t key = ticker_key(convertStringToTicker("SPY"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.asks.sharesAt(1000), 10);

    Order expected{.price=1000,
                   .shares=10,
                   .side=Side::Sell,
                   .timestamp=convertIntegerToTimestamp(1),
                   .stock=convertStringToTicker("SPY")
    };
    EXPECT_EQ(this->orderbook_manager.orders[2], expected);
}

TYPED_TEST(OrderBookManagerTest, HandleAddOrderMPIDAttributionMessage) {
    AddOrderMPIDAttributionMessage msg{.stock_locate=0,
                        .tracking_number=0,
                        .timestamp=convertIntegerToTimestamp(3),
                        .order_reference_number=4,
                        .side=Side::Buy,
                        .shares=15,
                        .stock=convertStringToTicker("MSFT"),
                        .price=2000,
                        .MPID=0
    };
    this->orderbook_manager.handle(msg);

    uint64_t key = ticker_key(convertStringToTicker("MSFT"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.bids.sharesAt(2000), 15);

    Order expected{.price=2000,
                   .shares=15,
                   .side=Side::Buy,
                   .timestamp=convertIntegerToTimestamp(3),
                   .stock=convertStringToTicker("MSFT")
    };
    EXPECT_EQ(this->orderbook_manager.orders[4], expected);
}

TYPED_TEST(OrderBookManagerTest, ProcessDispatchesToCorrectHandler) {
    AddOrderMessage bid{.stock_locate=0,
                        .tracking_number=0,
                        .timestamp=convertIntegerToTimestamp(5),
                        .order_reference_number=5,
                        .side=Side::Buy,
                        .shares=20,
                        .stock=convertStringToTicker("GOOG"),
                        .price=3000
    };
    Message msg{bid};
    this->orderbook_manager.process(msg);

    uint64_t key = ticker_key(convertStringToTicker("GOOG"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.bids.sharesAt(3000), 20);
    EXPECT_TRUE(this->orderbook_manager.orders.contains(5));
}

TYPED_TEST(OrderBookManagerTest, HandlePartialOrderExecutedBid) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .executed_shares=5,
        .match_number=0
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.bids.sharesAt(1000), 5);

    Order expected{.price=1000,
                   .shares=5,
                   .side=Side::Buy,
                   .timestamp=convertIntegerToTimestamp(0),
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(this->orderbook_manager.orders[1], expected);
}

TYPED_TEST(OrderBookManagerTest, HandlePartialOrderExecutedAsk) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=2,
        .executed_shares=5,
        .match_number=0
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("SPY"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.asks.sharesAt(1000), 5);

    Order expected{.price=1000,
                   .shares=5,
                   .side=Side::Sell,
                   .timestamp=convertIntegerToTimestamp(1),
                   .stock=convertStringToTicker("SPY")
    };
    EXPECT_EQ(this->orderbook_manager.orders[2], expected);
}

TYPED_TEST(OrderBookManagerTest, HandleFillOrderExecutedBid) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .executed_shares=10,
        .match_number=0
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));

    EXPECT_FALSE(this->orderbook_manager.orders.contains(1));
}

TYPED_TEST(OrderBookManagerTest, HandleFillOrderExecutedAsk) {
    OrderExecutedMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=2,
        .executed_shares=10,
        .match_number=0
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("SPY"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_FALSE(book.asks.contains(1000));

    EXPECT_FALSE(this->orderbook_manager.orders.contains(2));
}

TYPED_TEST(OrderBookManagerTest, HandleOrderExecutedPrice) {
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
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));

    EXPECT_FALSE(this->orderbook_manager.orders.contains(1));
}

TYPED_TEST(OrderBookManagerTest, HandleOrderCancel) {
    OrderCancelMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1,
        .cancelled_shares=5
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_EQ(book.bids.sharesAt(1000), 5);

    Order expected{.price=1000,
                   .shares=5,
                   .side=Side::Buy,
                   .timestamp=convertIntegerToTimestamp(0),
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(this->orderbook_manager.orders[1], expected);
}

TYPED_TEST(OrderBookManagerTest, HandleOrderReplace) {
    OrderReplaceMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .original_order_reference_number=1,
        .new_order_reference_number=3,
        .shares=30,
        .price=1010
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));
    EXPECT_EQ(book.bids.sharesAt(1010), 30);

    Order expected{.price=1010,
                   .shares=30,
                   .side=Side::Buy,
                   .timestamp=convertIntegerToTimestamp(2),
                   .stock=convertStringToTicker("AAPL")
    };
    EXPECT_EQ(this->orderbook_manager.orders[3], expected);
    EXPECT_FALSE(this->orderbook_manager.orders.contains(1));
}

TYPED_TEST(OrderBookManagerTest, HandleOrderDelete) {
    OrderDeleteMessage msg = {
        .stock_locate=0,
        .tracking_number=0,
        .timestamp=convertIntegerToTimestamp(2),
        .order_reference_number=1
    };
    this->orderbook_manager.handle(msg);
    uint64_t key = ticker_key(convertStringToTicker("AAPL"));
    TypeParam& book = this->orderbook_manager.books[key];
    EXPECT_FALSE(book.bids.contains(1000));
    EXPECT_FALSE(this->orderbook_manager.orders.contains(1));
}

TYPED_TEST(OrderBookManagerTest, ResetClearsBooksAndOrders) {
    this->orderbook_manager.reset();

    EXPECT_TRUE(this->orderbook_manager.books.empty());
    EXPECT_TRUE(this->orderbook_manager.orders.empty());
}
