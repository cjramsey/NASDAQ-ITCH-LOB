#pragma once

#include <memory>
#include <string>

#include <arrow/api.h>
#include <parquet/arrow/writer.h>

#include "types.h"

// Writes one message type to one Parquet file. Rows are buffered into an
// arrow::RecordBatchBuilder and flushed to disk every kFlushRows rows, so
// memory stays bounded regardless of input file size.
class TypedParquetWriter {
public:
    TypedParquetWriter(const std::string& path, std::shared_ptr<arrow::Schema> schema);

    arrow::RecordBatchBuilder& builder() { return *builder_; }
    void MaybeFlush();
    void Finish();

private:
    static constexpr int64_t kFlushRows = 1 << 16;

    void Flush();

    std::shared_ptr<arrow::Schema> schema_;
    std::unique_ptr<arrow::RecordBatchBuilder> builder_;
    std::unique_ptr<parquet::arrow::FileWriter> file_writer_;
    int64_t rows_since_flush_ = 0;
};

// Owns one TypedParquetWriter per exported message type and routes each
// decoded Message to the matching writer. Mirrors OrderbookManager's
// std::visit dispatch shape (include/lob.h) but writes rows instead of
// mutating an order book.
class ParquetExportManager {
public:
    explicit ParquetExportManager(const std::string& output_dir);

    void write(const Message& msg);
    void finish();

private:
    void write(const AddOrderMessage& msg);
    void write(const AddOrderMPIDAttributionMessage& msg);
    void write(const OrderExecutedMessage& msg);
    void write(const OrderExecutedPriceMessage& msg);
    void write(const OrderCancelMessage& msg);
    void write(const OrderDeleteMessage& msg);
    void write(const OrderReplaceMessage& msg);
    void write(const TradeMessage& msg);
    void write(const CrossTradeMessage& msg);
    void write(const BrokenTradeMessage& msg);
    void write(const std::monostate&) {};

    TypedParquetWriter add_order_;
    TypedParquetWriter add_order_mpid_;
    TypedParquetWriter order_executed_;
    TypedParquetWriter order_executed_price_;
    TypedParquetWriter order_cancel_;
    TypedParquetWriter order_delete_;
    TypedParquetWriter order_replace_;
    TypedParquetWriter trade_;
    TypedParquetWriter cross_trade_;
    TypedParquetWriter broken_trade_;
};
