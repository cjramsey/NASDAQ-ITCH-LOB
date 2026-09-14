#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "lob.h"
#include "parquet_writer.h"

// Emits one row per book-changing event: timestamp, stock, and the top N
// price levels per side, using OrderbookManager<BBOOrderbook> as the single
// source of truth for order->(stock,side,price) resolution and book state.
class DepthExportManager {
public:
    DepthExportManager(const std::string& output_dir, size_t levels);

    void write(const Message& msg);
    void finish();

private:
    template <typename Compare>
    void writeSide(int price_field, int size_field, const SortedVectorBook<Compare>& book);

    size_t levels_;
    OrderbookManager<BBOOrderbook> books_;
    TypedParquetWriter writer_;
    std::vector<PriceLevel> scratch_;   // reused per event, avoids reallocation
};
