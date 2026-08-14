#include <stdexcept>

#include <arrow/io/file.h>

#include "parquet_writer.h"

namespace {
    // Ticker is space-padded to 8 bytes on the wire; trim the padding before
    // storing it as a string column.
    std::string trim_ticker(const Ticker& stock) {
        std::string s(stock.data(), stock.size());
        auto end = s.find_last_not_of(' ');
        return (end == std::string::npos) ? std::string{} : s.substr(0, end + 1);
    }

    std::string side_to_string(Side side) {
        return std::string(1, static_cast<char>(side));
    }

    std::string path_for(const std::string& output_dir, const std::string& name) {
        return output_dir + "/" + name + ".parquet";
    }
}

TypedParquetWriter::TypedParquetWriter(const std::string& path, std::shared_ptr<arrow::Schema> schema)
    : schema_(std::move(schema))
{
    auto builder_result = arrow::RecordBatchBuilder::Make(schema_, arrow::default_memory_pool());
    if (!builder_result.ok())
        throw std::runtime_error("Failed to create RecordBatchBuilder for: " + path);
    builder_ = std::move(*builder_result);

    auto sink = arrow::io::FileOutputStream::Open(path);
    if (!sink.ok())
        throw std::runtime_error("Failed to open output file: " + path);

    auto writer = parquet::arrow::FileWriter::Open(*schema_, arrow::default_memory_pool(), *sink);
    if (!writer.ok())
        throw std::runtime_error("Failed to open Parquet writer for: " + path);
    file_writer_ = std::move(*writer);
}

void TypedParquetWriter::Flush() {
    auto batch_result = builder_->Flush();
    if (!batch_result.ok())
        throw std::runtime_error("Failed to flush record batch");

    auto batch = *batch_result;
    if (batch->num_rows() > 0 && !file_writer_->WriteRecordBatch(*batch).ok())
        throw std::runtime_error("Failed to write record batch");

    rows_since_flush_ = 0;
}

void TypedParquetWriter::MaybeFlush() {
    if (++rows_since_flush_ >= kFlushRows)
        Flush();
}

void TypedParquetWriter::Finish() {
    Flush();
    if (!file_writer_->Close().ok())
        throw std::runtime_error("Failed to close Parquet file");
}

ParquetExportManager::ParquetExportManager(const std::string& output_dir) :
    add_order_(path_for(output_dir, "add_order"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
        arrow::field("side", arrow::utf8()),
        arrow::field("shares", arrow::uint32()),
        arrow::field("stock", arrow::utf8()),
        arrow::field("price", arrow::uint32()),
    })),
    add_order_mpid_(path_for(output_dir, "add_order_mpid"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
        arrow::field("side", arrow::utf8()),
        arrow::field("shares", arrow::uint32()),
        arrow::field("stock", arrow::utf8()),
        arrow::field("price", arrow::uint32()),
        arrow::field("MPID", arrow::uint32()),
    })),
    order_executed_(path_for(output_dir, "order_executed"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
        arrow::field("executed_shares", arrow::uint32()),
        arrow::field("match_number", arrow::uint64()),
    })),
    order_executed_price_(path_for(output_dir, "order_executed_price"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
        arrow::field("executed_shares", arrow::uint32()),
        arrow::field("match_number", arrow::uint64()),
        arrow::field("printable", arrow::utf8()),
        arrow::field("execution_price", arrow::uint32()),
    })),
    order_cancel_(path_for(output_dir, "order_cancel"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
        arrow::field("cancelled_shares", arrow::uint32()),
    })),
    order_delete_(path_for(output_dir, "order_delete"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
    })),
    order_replace_(path_for(output_dir, "order_replace"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("original_order_reference_number", arrow::uint64()),
        arrow::field("new_order_reference_number", arrow::uint64()),
        arrow::field("shares", arrow::uint32()),
        arrow::field("price", arrow::uint32()),
    })),
    trade_(path_for(output_dir, "trade"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("order_reference_number", arrow::uint64()),
        arrow::field("side", arrow::utf8()),
        arrow::field("shares", arrow::uint32()),
        arrow::field("stock", arrow::utf8()),
        arrow::field("price", arrow::uint32()),
        arrow::field("match_number", arrow::uint64()),
    })),
    cross_trade_(path_for(output_dir, "cross_trade"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("shares", arrow::uint64()),
        arrow::field("stock", arrow::utf8()),
        arrow::field("cross_price", arrow::uint32()),
        arrow::field("match_number", arrow::uint64()),
        arrow::field("cross_type", arrow::utf8()),
    })),
    broken_trade_(path_for(output_dir, "broken_trade"), arrow::schema({
        arrow::field("stock_locate", arrow::uint16()),
        arrow::field("tracking_number", arrow::uint16()),
        arrow::field("timestamp_ns", arrow::uint64()),
        arrow::field("match_number", arrow::uint64()),
    }))
{}

void ParquetExportManager::write(const Message& msg) {
    std::visit([this](const auto& m) { write(m); }, msg);
}

void ParquetExportManager::finish() {
    add_order_.Finish();
    add_order_mpid_.Finish();
    order_executed_.Finish();
    order_executed_price_.Finish();
    order_cancel_.Finish();
    order_delete_.Finish();
    order_replace_.Finish();
    trade_.Finish();
    cross_trade_.Finish();
    broken_trade_.Finish();
}

void ParquetExportManager::write(const AddOrderMessage& msg) {
    auto& b = add_order_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    b.GetFieldAs<arrow::StringBuilder>(4)->Append(side_to_string(msg.side));
    b.GetFieldAs<arrow::UInt32Builder>(5)->Append(msg.shares);
    b.GetFieldAs<arrow::StringBuilder>(6)->Append(trim_ticker(msg.stock));
    b.GetFieldAs<arrow::UInt32Builder>(7)->Append(msg.price);
    add_order_.MaybeFlush();
}

void ParquetExportManager::write(const AddOrderMPIDAttributionMessage& msg) {
    auto& b = add_order_mpid_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    b.GetFieldAs<arrow::StringBuilder>(4)->Append(side_to_string(msg.side));
    b.GetFieldAs<arrow::UInt32Builder>(5)->Append(msg.shares);
    b.GetFieldAs<arrow::StringBuilder>(6)->Append(trim_ticker(msg.stock));
    b.GetFieldAs<arrow::UInt32Builder>(7)->Append(msg.price);
    b.GetFieldAs<arrow::UInt32Builder>(8)->Append(msg.MPID);
    add_order_mpid_.MaybeFlush();
}

void ParquetExportManager::write(const OrderExecutedMessage& msg) {
    auto& b = order_executed_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    b.GetFieldAs<arrow::UInt32Builder>(4)->Append(msg.executed_shares);
    b.GetFieldAs<arrow::UInt64Builder>(5)->Append(msg.match_number);
    order_executed_.MaybeFlush();
}

void ParquetExportManager::write(const OrderExecutedPriceMessage& msg) {
    auto& b = order_executed_price_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    b.GetFieldAs<arrow::UInt32Builder>(4)->Append(msg.executed_shares);
    b.GetFieldAs<arrow::UInt64Builder>(5)->Append(msg.match_number);
    b.GetFieldAs<arrow::StringBuilder>(6)->Append(std::string(1, msg.printable));
    b.GetFieldAs<arrow::UInt32Builder>(7)->Append(msg.execution_price);
    order_executed_price_.MaybeFlush();
}

void ParquetExportManager::write(const OrderCancelMessage& msg) {
    auto& b = order_cancel_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    b.GetFieldAs<arrow::UInt32Builder>(4)->Append(msg.cancelled_shares);
    order_cancel_.MaybeFlush();
}

void ParquetExportManager::write(const OrderDeleteMessage& msg) {
    auto& b = order_delete_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    order_delete_.MaybeFlush();
}

void ParquetExportManager::write(const OrderReplaceMessage& msg) {
    auto& b = order_replace_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.original_order_reference_number);
    b.GetFieldAs<arrow::UInt64Builder>(4)->Append(msg.new_order_reference_number);
    b.GetFieldAs<arrow::UInt32Builder>(5)->Append(msg.shares);
    b.GetFieldAs<arrow::UInt32Builder>(6)->Append(msg.price);
    order_replace_.MaybeFlush();
}

void ParquetExportManager::write(const TradeMessage& msg) {
    auto& b = trade_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.order_reference_number);
    b.GetFieldAs<arrow::StringBuilder>(4)->Append(side_to_string(msg.side));
    b.GetFieldAs<arrow::UInt32Builder>(5)->Append(msg.shares);
    b.GetFieldAs<arrow::StringBuilder>(6)->Append(trim_ticker(msg.stock));
    b.GetFieldAs<arrow::UInt32Builder>(7)->Append(msg.price);
    b.GetFieldAs<arrow::UInt64Builder>(8)->Append(msg.match_number);
    trade_.MaybeFlush();
}

void ParquetExportManager::write(const CrossTradeMessage& msg) {
    auto& b = cross_trade_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.shares);
    b.GetFieldAs<arrow::StringBuilder>(4)->Append(trim_ticker(msg.stock));
    b.GetFieldAs<arrow::UInt32Builder>(5)->Append(msg.cross_price);
    b.GetFieldAs<arrow::UInt64Builder>(6)->Append(msg.match_number);
    b.GetFieldAs<arrow::StringBuilder>(7)->Append(std::string(1, msg.cross_type));
    cross_trade_.MaybeFlush();
}

void ParquetExportManager::write(const BrokenTradeMessage& msg) {
    auto& b = broken_trade_.builder();
    b.GetFieldAs<arrow::UInt16Builder>(0)->Append(msg.stock_locate);
    b.GetFieldAs<arrow::UInt16Builder>(1)->Append(msg.tracking_number);
    b.GetFieldAs<arrow::UInt64Builder>(2)->Append(parse_timestamp(msg.timestamp));
    b.GetFieldAs<arrow::UInt64Builder>(3)->Append(msg.match_number);
    broken_trade_.MaybeFlush();
}
