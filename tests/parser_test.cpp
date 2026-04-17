#include <gtest/gtest.h>

#include <parser.h>


TEST(ITCHParserTest, ParseAddOrderMessage) {
    std::vector<std::byte> raw_bytes = parser::build_add_order_bytes(1, 1, Side::Buy, 25, "AAPL", 1000);
    AddOrderMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .order_reference_number=1,
        .side=Side::Buy,
        .shares=25,
        .stock=convertStringToTicker("AAPL"),
        .price=1000
    };
    EXPECT_EQ(parser::parse_add_order(&raw_bytes[0]), expected);
}

TEST(ITCHParserTest, ParseAddOrderMPIDMessage) {
    std::vector<std::byte> raw_bytes = parser::build_add_order_mpid_bytes(1, 1, Side::Buy, 25, "AAPL", 1000, 9384);
    AddOrderMPIDAttributionMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .order_reference_number=1,
        .side=Side::Buy,
        .shares=25,
        .stock=convertStringToTicker("AAPL"),
        .price=1000,
        .MPID=9384
    };
    EXPECT_EQ(parser::parse_add_order_mpid(&raw_bytes[0]), expected);
}

TEST(ITCHParserTest, ParseExecuteOrderMessage) {
    std::vector<std::byte> raw_bytes = parser::build_execute_order_bytes(1, 1, 50, 831479);
    OrderExecutedMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .order_reference_number=1,
        .executed_shares=50,
        .match_number=831479
    };
    EXPECT_EQ(parser::parse_order_executed(&raw_bytes[0]), expected);
}

TEST(ITCHParserTest, ParseExecuteOrderPriceMessage) {
    std::vector<std::byte> raw_bytes = parser::build_execute_price_order_bytes(1, 1, 20, 123456789, 1000);
    OrderExecutedPriceMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .order_reference_number=1,
        .executed_shares=20,
        .match_number=123456789,
        .execution_price=1000
    };
    EXPECT_EQ(parser::parse_order_executed_price(&raw_bytes[0]), expected);
}

TEST(ITCHParserTest, ParseCancelOrderMessage) {
    std::vector<std::byte> raw_bytes = parser::build_cancel_order_bytes(1, 1, 13784);
    OrderCancelMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .order_reference_number=1,
        .cancelled_shares=13784
    };
    EXPECT_EQ(parser::parse_order_cancel(&raw_bytes[0]), expected);
}

TEST(ITCHParserTest, ParseDeleteOrderMessage) {
    std::vector<std::byte> raw_bytes = parser::build_delete_order_bytes(1, 1);
    OrderDeleteMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .order_reference_number=1
    };
    EXPECT_EQ(parser::parse_order_delete(&raw_bytes[0]), expected);
}

TEST(ITCHParserTest, ParseReplaceOrderMessage) {
    std::vector<std::byte> raw_bytes = parser::build_replace_order_bytes(1, 1, 2, 100, 2000);
    OrderReplaceMessage expected{
        .timestamp=convertIntegerToTimestamp(1), 
        .original_order_reference_number=1,
        .new_order_reference_number=2,
        .shares=100,
        .price=2000
    };
    EXPECT_EQ(parser::parse_order_replace(&raw_bytes[0]), expected);
}