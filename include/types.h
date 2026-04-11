#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <variant>

enum class Side : char { Buy = 'B', Sell = 'S' };

// Used in switch case/dispatch table for identifying message types
namespace MessageType {
    constexpr char SystemEvent = 'S';
    constexpr char StockDirectory = 'R';
    constexpr char StockTradingAction = 'H';
    constexpr char RegSHORestriction = 'Y';
    constexpr char MarketParticipantPosition = 'L';
    constexpr char MWCBDecline = 'V';
    constexpr char MWCBStatus = 'W';
    constexpr char IPOQuotingPeriodUpdate = 'K';
    constexpr char LULDAuctionCollar = 'J';
    constexpr char OperationalHalt = 'h';
    constexpr char AddOrder = 'A';
    constexpr char AddOrderMPIDAttribution = 'F';
    constexpr char OrderExecuted = 'E';
    constexpr char OrderExecutedPrice = 'C';
    constexpr char OrderCancel = 'X';
    constexpr char OrderDelete = 'D';
    constexpr char OrderReplace = 'U';
    constexpr char Trade = 'P';
    constexpr char CrossTrade = 'Q';
    constexpr char BrokenTrade = 'B';
    constexpr char NOII = 'I';
    constexpr char RetailInterest = 'N';
    constexpr char DirectListingCapitalRaise = 'O';
}

template<char MessageType_t> 
constexpr uint16_t MessageLength = 0;

// Message lengths are for checking struct sizes 
template<> constexpr uint16_t MessageLength<MessageType::SystemEvent> = 12;
template<> constexpr uint16_t MessageLength<MessageType::StockDirectory> = 39;
template<> constexpr uint16_t MessageLength<MessageType::StockTradingAction> = 25;
template<> constexpr uint16_t MessageLength<MessageType::RegSHORestriction> = 20;
template<> constexpr uint16_t MessageLength<MessageType::MarketParticipantPosition> = 26;
template<> constexpr uint16_t MessageLength<MessageType::MWCBDecline> = 35;
template<> constexpr uint16_t MessageLength<MessageType::MWCBStatus> = 12;
template<> constexpr uint16_t MessageLength<MessageType::IPOQuotingPeriodUpdate> = 28;
template<> constexpr uint16_t MessageLength<MessageType::LULDAuctionCollar> = 35;
template<> constexpr uint16_t MessageLength<MessageType::OperationalHalt> = 21;
template<> constexpr uint16_t MessageLength<MessageType::AddOrder> = 36;
template<> constexpr uint16_t MessageLength<MessageType::AddOrderMPIDAttribution> = 40;
template<> constexpr uint16_t MessageLength<MessageType::OrderExecuted> = 31;
template<> constexpr uint16_t MessageLength<MessageType::OrderExecutedPrice> = 36;
template<> constexpr uint16_t MessageLength<MessageType::OrderCancel> = 23;
template<> constexpr uint16_t MessageLength<MessageType::OrderDelete> = 19;
template<> constexpr uint16_t MessageLength<MessageType::OrderReplace> = 35;
template<> constexpr uint16_t MessageLength<MessageType::Trade> = 44;
template<> constexpr uint16_t MessageLength<MessageType::CrossTrade> = 40;
template<> constexpr uint16_t MessageLength<MessageType::BrokenTrade> = 19;
template<> constexpr uint16_t MessageLength<MessageType::NOII> = 50;
template<> constexpr uint16_t MessageLength<MessageType::RetailInterest> = 20;
template<> constexpr uint16_t MessageLength<MessageType::DirectListingCapitalRaise> = 48;

// Remove padding/alignment from structs
#pragma pack(push, 1)

/* 
The messages parsed are:
    1. AddOrderMessage
    2. AddOrderMPIDMessage
    3. OrderExecutedMessage
    4. OrderExecutedPriceMessage
    5. OrderCancelMessage
    6. OrderDeleteMessage
    7. OrderReplaceMessage

The rest are ignored as they are not applicable to the toy orderbook, e.g. Market-Wide Circuit Breaker messages.
*/

struct SystemEventMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;   // change to raw array if std::array causes any issues when parsing
    char event_code;
};

struct StockDirectoryMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;   // consider using type alias
    std::array<char, 8> stock;
    char market_category;
    char financial_status_indicator;
    uint32_t round_lot_size;
    char round_lots_only;
    char issue_clarification;
    std::array<char, 2> issue_sub_type;
    char authenticity;
    char short_sale_threshold_indicator;
    char IPO_flag;
    char LULD_reference_price_tier;
    char ETP_flag;
    uint32_t ETP_leverage_factor; 
    char inverse_indicator;
};

struct StockTradingActionMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    char trading_state;
    char reserved;
    std::array<char, 4> reason; 
};

struct RegSHORestrictionMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    char reg_SHO_action;
};

struct MarketParticipantPositionMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 4> MPID;
    std::array<char, 8> stock;
    char primary_market_maker;
    char market_maker_mode;
    char market_participant_mode;
};

struct MWCBDeclineMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t level1;
    uint64_t level2;
    uint64_t level3;
};

struct MWCBStatusMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    char breached_level;
};

struct IPOQuotingPeriodUpdateMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    uint32_t IPO_quotation_release_time;
    char IPO_quotation_release_qualifier;
    uint32_t IPO_price;
};

struct LULDAuctionCollarMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    uint32_t auction_collar_ref_price;
    uint32_t upper_auction_collar_price;
    uint32_t lower_auction_collar_price;
    uint32_t auction_collar_extension;
};

struct OperationalHaltMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    char market_code;
    char operational_halt_action;
};

struct AddOrderMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    Side side;
    uint32_t shares;
    std::array<char, 8> stock;
    uint32_t price;
};

struct AddOrderMPIDAttributionMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    Side side;
    uint32_t shares;
    std::array<char, 8> stock;
    uint32_t price;
    uint32_t MPID;
};

struct OrderExecutedMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
};

struct OrderExecutedPriceMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    uint32_t executed_shares;
    uint64_t match_number;
    char printable;
    uint32_t execution_price;
};

struct OrderCancelMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    uint32_t cancelled_shares;
};

struct OrderDeleteMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
};

struct OrderReplaceMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t original_order_reference_number;
    uint64_t new_order_reference_number;
    uint32_t shares;
    uint32_t price;
};

struct TradeMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    Side side;
    uint32_t shares;
    std::array<char, 8> stock;
    uint32_t price;
    uint64_t match_number;
};

struct CrossTradeMessage { 
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t order_reference_number;
    Side side;
    uint64_t shares;
    std::array<char, 8> stock;
    uint32_t cross_price;
};

struct BrokenTradeMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t match_number;
};

struct NOIIMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    uint64_t paired_shares;
    uint64_t imbalance_shares;
    char imbalance_direction;
    std::array<char, 8> stock;
    uint32_t far_price;
    uint32_t near_price;
    uint32_t current_reference_price;
    char cross_type;
    char price_variation_indicator;
};

struct RetailInterestMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    char interest_flag;
};

struct DirectListingCapitalRaiseMessage {
    uint16_t stock_locate;
    uint16_t tracking_number;
    std::array<uint8_t, 6> timestamp;
    std::array<char, 8> stock;
    char open_eligibility_status;
    uint32_t min_allowable_price;
    uint32_t max_allowable_price;
    uint32_t near_execution_price;
    uint64_t near_execution_time;
    uint32_t lower_price_range_collar;
    uint32_t upper_price_range_collar;
};

#pragma pack(pop)

// Check size of structs are as intended, subtract one as the character symbol not included in structs
static_assert(sizeof(SystemEventMessage) == MessageLength<MessageType::SystemEvent> - 1);
static_assert(sizeof(StockDirectoryMessage) == MessageLength<MessageType::StockDirectory> - 1);
static_assert(sizeof(StockTradingActionMessage) == MessageLength<MessageType::StockTradingAction> - 1);
static_assert(sizeof(RegSHORestrictionMessage) == MessageLength<MessageType::RegSHORestriction> - 1);
static_assert(sizeof(MarketParticipantPositionMessage) == MessageLength<MessageType::MarketParticipantPosition> - 1);
static_assert(sizeof(MWCBDeclineMessage) == MessageLength<MessageType::MWCBDecline> - 1);
static_assert(sizeof(MWCBStatusMessage) == MessageLength<MessageType::MWCBStatus> - 1);
static_assert(sizeof(IPOQuotingPeriodUpdateMessage) == MessageLength<MessageType::IPOQuotingPeriodUpdate> - 1);
static_assert(sizeof(LULDAuctionCollarMessage) == MessageLength<MessageType::LULDAuctionCollar> - 1);
static_assert(sizeof(OperationalHaltMessage) == MessageLength<MessageType::OperationalHalt> - 1);
static_assert(sizeof(AddOrderMessage) == MessageLength<MessageType::AddOrder> - 1);
static_assert(sizeof(AddOrderMPIDAttributionMessage) == MessageLength<MessageType::AddOrderMPIDAttribution> - 1);
static_assert(sizeof(OrderExecutedMessage) == MessageLength<MessageType::OrderExecuted> - 1);
static_assert(sizeof(OrderExecutedPriceMessage) == MessageLength<MessageType::OrderExecutedPrice> - 1);
static_assert(sizeof(OrderCancelMessage) == MessageLength<MessageType::OrderCancel> - 1);
static_assert(sizeof(OrderDeleteMessage) == MessageLength<MessageType::OrderDelete> - 1);
static_assert(sizeof(OrderReplaceMessage) == MessageLength<MessageType::OrderReplace> - 1);
static_assert(sizeof(TradeMessage) == MessageLength<MessageType::Trade> - 1);
static_assert(sizeof(CrossTradeMessage) == MessageLength<MessageType::CrossTrade> - 1);
static_assert(sizeof(BrokenTradeMessage) == MessageLength<MessageType::BrokenTrade> - 1);
static_assert(sizeof(NOIIMessage) == MessageLength<MessageType::NOII> - 1);
static_assert(sizeof(RetailInterestMessage) == MessageLength<MessageType::RetailInterest> - 1);
static_assert(sizeof(DirectListingCapitalRaiseMessage) == MessageLength<MessageType::DirectListingCapitalRaise> - 1);

//Helper functions

uint64_t parse_timestamp(const std::array<uint8_t, 6>& timestamp);

std::string parse_stock(const std::array<char, 8>& stock);

// Printing declarations
std::ostream& operator<<(std::ostream& os, const AddOrderMessage& msg);
std::ostream& operator<<(std::ostream& os, const AddOrderMPIDAttributionMessage& msg);
std::ostream& operator<<(std::ostream& os, const OrderExecutedMessage& msg);
std::ostream& operator<<(std::ostream& os, const OrderExecutedPriceMessage& msg);
std::ostream& operator<<(std::ostream& os, const OrderCancelMessage& msg);
std::ostream& operator<<(std::ostream& os, const OrderDeleteMessage& msg);
std::ostream& operator<<(std::ostream& os, const OrderReplaceMessage& msg);


using Message = std::variant<
    AddOrderMessage,
    AddOrderMPIDAttributionMessage,
    OrderExecutedMessage,
    OrderExecutedPriceMessage,
    OrderCancelMessage,
    OrderDeleteMessage,
    OrderReplaceMessage,
    std::monostate
>;