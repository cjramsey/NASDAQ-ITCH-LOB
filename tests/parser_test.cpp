#include <gtest/gtest.h>

#include <endian.h>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <parser.h>

namespace {
    void append_message(std::vector<std::byte>& buf, char type, const std::vector<std::byte>& body, uint16_t length) {
        uint16_t len_be = htobe16(length);
        const auto* len_bytes = reinterpret_cast<const std::byte*>(&len_be);
        buf.insert(buf.end(), len_bytes, len_bytes + sizeof(len_be));
        buf.push_back(static_cast<std::byte>(type));
        buf.insert(buf.end(), body.begin(), body.end());
    }
}

// A small file (well under one 64KB read chunk) whose last messages fall
// within read_messages' lazy MAX_MSG_SIZE stopping margin, so they'd only
// ever get processed if EOF correctly drains the leftover buffer.
TEST(ITCHReaderTest, ProcessesTrailingMessagesAtEndOfFile) {
    std::vector<std::byte> file_bytes;

    auto add_body = parser::build_add_order_bytes(1, 42, Side::Buy, 100, "AAPL", 500);
    append_message(file_bytes, MessageType::AddOrder, add_body, MessageLength<MessageType::AddOrder>);

    std::vector<std::byte> sys_body(sizeof(SystemEventMessage), std::byte{0});
    append_message(file_bytes, MessageType::SystemEvent, sys_body, MessageLength<MessageType::SystemEvent>);

    auto delete_body = parser::build_delete_order_bytes(2, 42);
    append_message(file_bytes, MessageType::OrderDelete, delete_body, MessageLength<MessageType::OrderDelete>);

    auto path = std::filesystem::temp_directory_path() / "itch_reader_test.bin";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(file_bytes.data()), file_bytes.size());
    }

    std::vector<Message> received;
    uint64_t counter{};
    {
        ITCHReader reader{path.string()};
        reader.read_messages([&received](Message&& msg) {
            received.push_back(msg);
        }, counter);
    }
    std::filesystem::remove(path);

    EXPECT_EQ(counter, 3);         // AddOrder, SystemEvent (skipped), OrderDelete
    ASSERT_EQ(received.size(), 2); // only the 2 handled types reach process()
    EXPECT_TRUE(std::holds_alternative<AddOrderMessage>(received[0]));
    EXPECT_TRUE(std::holds_alternative<OrderDeleteMessage>(received[1]));
}

// Exercises every handled message type through read_messages' actual dispatch
// switch, not just the parser::parse_* functions directly.
TEST(ITCHReaderTest, DispatchesEveryHandledMessageType) {
    std::vector<std::byte> file_bytes;

    append_message(file_bytes, MessageType::AddOrder,
        parser::build_add_order_bytes(1, 1, Side::Buy, 25, "AAPL", 1000),
        MessageLength<MessageType::AddOrder>);
    append_message(file_bytes, MessageType::AddOrderMPIDAttribution,
        parser::build_add_order_mpid_bytes(1, 2, Side::Sell, 25, "AAPL", 1000, 9384),
        MessageLength<MessageType::AddOrderMPIDAttribution>);
    append_message(file_bytes, MessageType::OrderExecuted,
        parser::build_execute_order_bytes(1, 1, 5, 831479),
        MessageLength<MessageType::OrderExecuted>);
    append_message(file_bytes, MessageType::OrderExecutedPrice,
        parser::build_execute_price_order_bytes(1, 2, 5, 123456789, 1000),
        MessageLength<MessageType::OrderExecutedPrice>);
    append_message(file_bytes, MessageType::OrderCancel,
        parser::build_cancel_order_bytes(1, 1, 5),
        MessageLength<MessageType::OrderCancel>);
    append_message(file_bytes, MessageType::OrderReplace,
        parser::build_replace_order_bytes(1, 1, 3, 100, 2000),
        MessageLength<MessageType::OrderReplace>);
    append_message(file_bytes, MessageType::OrderDelete,
        parser::build_delete_order_bytes(1, 2),
        MessageLength<MessageType::OrderDelete>);

    auto path = std::filesystem::temp_directory_path() / "itch_reader_dispatch_test.bin";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(reinterpret_cast<const char*>(file_bytes.data()), file_bytes.size());
    }

    std::vector<Message> received;
    uint64_t counter{};
    {
        ITCHReader reader{path.string()};
        reader.read_messages([&received](Message&& msg) {
            received.push_back(msg);
        }, counter);
    }
    std::filesystem::remove(path);

    ASSERT_EQ(received.size(), 7);
    EXPECT_TRUE(std::holds_alternative<AddOrderMessage>(received[0]));
    EXPECT_TRUE(std::holds_alternative<AddOrderMPIDAttributionMessage>(received[1]));
    EXPECT_TRUE(std::holds_alternative<OrderExecutedMessage>(received[2]));
    EXPECT_TRUE(std::holds_alternative<OrderExecutedPriceMessage>(received[3]));
    EXPECT_TRUE(std::holds_alternative<OrderCancelMessage>(received[4]));
    EXPECT_TRUE(std::holds_alternative<OrderReplaceMessage>(received[5]));
    EXPECT_TRUE(std::holds_alternative<OrderDeleteMessage>(received[6]));
}


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


namespace {
    template <typename T>
    std::string printed(const T& msg) {
        std::ostringstream os;
        os << msg;
        return os.str();
    }
}

TEST(ITCHMessagePrintTest, AddOrderMessage) {
    AddOrderMessage msg{.order_reference_number=1, .side=Side::Buy, .shares=25,
                         .stock=convertStringToTicker("AAPL"), .price=1000};
    std::string out = printed(msg);
    EXPECT_NE(out.find("buy"), std::string::npos);
    EXPECT_NE(out.find("25"), std::string::npos);
    EXPECT_NE(out.find("AAPL"), std::string::npos);
    EXPECT_NE(out.find("1000"), std::string::npos);
}

TEST(ITCHMessagePrintTest, AddOrderMPIDAttributionMessage) {
    AddOrderMPIDAttributionMessage msg{.order_reference_number=1, .side=Side::Sell, .shares=25,
                                        .stock=convertStringToTicker("AAPL"), .price=1000, .MPID=9384};
    std::string out = printed(msg);
    EXPECT_NE(out.find("sell"), std::string::npos);
    EXPECT_NE(out.find("9384"), std::string::npos);
}

TEST(ITCHMessagePrintTest, OrderExecutedMessage) {
    OrderExecutedMessage msg{.order_reference_number=1, .executed_shares=50, .match_number=831479};
    std::string out = printed(msg);
    EXPECT_NE(out.find("50"), std::string::npos);
    EXPECT_NE(out.find("831479"), std::string::npos);
}

TEST(ITCHMessagePrintTest, OrderExecutedPriceMessage) {
    OrderExecutedPriceMessage msg{.order_reference_number=1, .executed_shares=20,
                                   .match_number=123456789, .printable='Y', .execution_price=1000};
    std::string out = printed(msg);
    EXPECT_NE(out.find("20"), std::string::npos);
    EXPECT_NE(out.find("123456789"), std::string::npos);
    EXPECT_NE(out.find("Y"), std::string::npos);
}

TEST(ITCHMessagePrintTest, OrderCancelMessage) {
    OrderCancelMessage msg{.order_reference_number=1, .cancelled_shares=13784};
    std::string out = printed(msg);
    EXPECT_NE(out.find("13784"), std::string::npos);
}

TEST(ITCHMessagePrintTest, OrderDeleteMessage) {
    OrderDeleteMessage msg{.order_reference_number=42};
    std::string out = printed(msg);
    EXPECT_NE(out.find("42"), std::string::npos);
}

TEST(ITCHMessagePrintTest, OrderReplaceMessage) {
    OrderReplaceMessage msg{.original_order_reference_number=1, .new_order_reference_number=2,
                             .shares=100, .price=2000};
    std::string out = printed(msg);
    EXPECT_NE(out.find("100"), std::string::npos);
    EXPECT_NE(out.find("2000"), std::string::npos);
}