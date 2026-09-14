#include <cstdio>
#include <optional>
#include <type_traits>
#include <utility>

#include "depth_writer.h"

namespace {
    // Every parsed message type has a `.timestamp` field except std::monostate.
    Timestamp extract_timestamp(const Message& msg) {
        return std::visit([](const auto& m) -> Timestamp {
            if constexpr (requires { m.timestamp; }) return m.timestamp;
            else return {};
        }, msg);
    }

    std::shared_ptr<arrow::Schema> make_schema(size_t levels) {
        arrow::FieldVector fields{
            arrow::field("timestamp_ns", arrow::uint64()),
            arrow::field("stock", arrow::utf8()),
        };
        auto add = [&](const char* prefix, const std::shared_ptr<arrow::DataType>& type) {
            for (size_t i = 0; i < levels; ++i) {
                char name[16];
                std::snprintf(name, sizeof(name), "%s_%02zu", prefix, i);
                fields.push_back(arrow::field(name, type));
            }
        };
        add("bid_px", arrow::uint32());   // fields 2 .. 2+L-1, level 00 = best bid
        add("bid_sz", arrow::uint64());   // fields 2+L .. 2+2L-1
        add("ask_px", arrow::uint32());   // fields 2+2L .. , level 00 = best ask
        add("ask_sz", arrow::uint64());   // fields 2+3L ..
        return arrow::schema(fields);
    }

    // Resolve (ticker_key, Ticker) for the stock a message affects, BEFORE
    // process() mutates/erases order state. Add* carries it directly;
    // Cancel/Delete/Executed*/Replace must look it up via `orders`
    // (mirrors the lookups OrderbookManager::handle() already does internally).
    std::optional<std::pair<uint64_t, Ticker>> resolve_stock(
        const Message& msg, const ankerl::unordered_dense::map<uint64_t, Order>& orders)
    {
        return std::visit([&](const auto& m) -> std::optional<std::pair<uint64_t, Ticker>> {
            using T = std::decay_t<decltype(m)>;
            if constexpr (std::is_same_v<T, AddOrderMessage> || std::is_same_v<T, AddOrderMPIDAttributionMessage>) {
                return std::pair{ticker_key(m.stock), m.stock};
            } else if constexpr (std::is_same_v<T, OrderCancelMessage> || std::is_same_v<T, OrderDeleteMessage> ||
                                 std::is_same_v<T, OrderExecutedMessage> || std::is_same_v<T, OrderExecutedPriceMessage>) {
                auto it = orders.find(m.order_reference_number);
                if (it == orders.end()) return std::nullopt;
                return std::pair{ticker_key(it->second.stock), it->second.stock};
            } else if constexpr (std::is_same_v<T, OrderReplaceMessage>) {
                auto it = orders.find(m.original_order_reference_number);
                if (it == orders.end()) return std::nullopt;
                return std::pair{ticker_key(it->second.stock), it->second.stock};
            } else {
                return std::nullopt;   // Trade/CrossTrade/BrokenTrade/monostate: doesn't move the book
            }
        }, msg);
    }
}

DepthExportManager::DepthExportManager(const std::string& output_dir, size_t levels)
    : levels_(levels), writer_(output_dir + "/depth.parquet", make_schema(levels))
{}

void DepthExportManager::write(const Message& msg) {
    auto resolved = resolve_stock(msg, books_.orders);
    auto ts = extract_timestamp(msg);
    books_.process(msg);
    if (!resolved) return;

    auto& [key, stock] = *resolved;
    auto& book = books_.books[key];
    auto& b = writer_.builder();
    b.GetFieldAs<arrow::UInt64Builder>(0)->Append(parse_timestamp(ts));
    b.GetFieldAs<arrow::StringBuilder>(1)->Append(parquet_export::trim_ticker(stock));
    const int L = static_cast<int>(levels_);
    writeSide(2,         2 + L,     book.bids);
    writeSide(2 + 2 * L, 2 + 3 * L, book.asks);
    writer_.MaybeFlush();
}

template <typename Compare>
void DepthExportManager::writeSide(int price_field, int size_field, const SortedVectorBook<Compare>& book) {
    book.topLevels(levels_, scratch_);
    auto& b = writer_.builder();
    for (int i = 0; i < static_cast<int>(levels_); ++i) {
        auto* prices = b.GetFieldAs<arrow::UInt32Builder>(price_field + i);
        auto* sizes = b.GetFieldAs<arrow::UInt64Builder>(size_field + i);
        if (static_cast<size_t>(i) < scratch_.size()) {
            prices->Append(scratch_[i].price);
            sizes->Append(scratch_[i].shares);
        } else {
            prices->AppendNull();
            sizes->AppendNull();
        }
    }
}

// Explicit instantiation for the two book sides (BBOOrderbook's bid/ask
// Compare types), same pattern as the explicit instantiations in lob.cpp.
template void DepthExportManager::writeSide(int, int, const SortedVectorBook<std::greater<uint32_t>>&);
template void DepthExportManager::writeSide(int, int, const SortedVectorBook<std::less<uint32_t>>&);

void DepthExportManager::finish() {
    writer_.Finish();
}
