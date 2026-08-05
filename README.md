
# NASDAQ ITCH 5.0 - Parser & Limit Order Book Reconstructor
 
A C++23 NASDAQ TotalView-ITCH 5.0 binary feed parser with full limit order book (LOB) reconstruction across all tracked instruments — built to explore low-latency feed handling, cache-conscious data structures, and market microstructure on real exchange data.


## Key Highlights

- End-to-end NASDAQ ITCH 5.0 parser and limit order book reconstruction on real exchange data  
- ~9.4 ns/message decode-only throughput on 100MB; 320 ns/message full parse + order-book reconstruction across a real ~13GB, 423M-message trading day  
- Explores trade-offs between direct processing and lock-free SPSC queue architectures  
- Designed with attention to memory layout, cache behaviour, and minimal allocation  

 
## Contents

1. [Overview](#overview)
2. [Performance](#performance)
3. [Project Structure](#project-structure)
4. [Design](#design)
5. [Message Handling](#message--handling)
6. [Build](#build)
7. [Usage](#usage)
8. [Testing](#testing)
9. [Roadmap](#roadmap)
10. [References](#references)

 
## Overview
 
NASDAQ's TotalView-ITCH 5.0 protocol delivers the complete order-by-order history of the exchange as a raw binary stream: every add, cancel, delete, execution, and replace, for every security, across an entire trading day. This project parses that stream and reconstructs the per-symbol limit order book in real time, maintaining aggregated quantity at each price level.
 
Two execution modes are provided: a direct single-threaded mode and a producer-consumer mode via a lock-free SPSC ring buffer, allowing the parser and order book update logic to be decoupled across threads.

ITCH Feed → Parser → (optional SPSC Ring Buffer) → Order Book Engine


## Performance
 
Measured on an Intel Core i7-1165G7 @ 2.80 GHz, 16GB RAM, `-O3 -march=native` with LTO. 10MB/100MB/1GB figures are Google Benchmark means over 5 repetitions; full-day is a single pass over a real ~13GB, 423M-message trading day (too large to repeat cheaply).

| Stage | 10 MB | 100 MB | 1 GB | Full day (~13 GB) |
|---|---|---|---|---|
| Parse only | 5.8 ns/msg | 9.4 ns/msg | 13.5 ns/msg | — |
| Parse + LOB update (`FastOrderbook`) | 29.2 ns/msg | 79.1 ns/msg | 219.0 ns/msg | 320 ns/msg |
| Parse + LOB update (`BBOOrderbook`) | 27.8 ns/msg | 73.4 ns/msg | 181.0 ns/msg | 316ns/msg |
| Ring buffer pipeline (`FastOrderbook`) | 42.9 ns/msg | 134.5 ns/msg | 297.7 ns/msg | 426 ns/msg |
| Ring buffer pipeline (`BBOOrderbook`) | 48.6 ns/msg | 137.0 ns/msg | 255.4 ns/msg | 396ns/msg |

Latency grows with dataset size on every LOB-touching benchmark as the order index and price maps outgrow L2/L3 cache; decode-only parsing stays flat since it never touches those structures. The ring buffer is slower than direct processing at every tier — synchronisation overhead outweighs the pipelining benefit here.

**Running benchmarks**

```bash
./build/benchmarks/bench --benchmark_filter=<regex>
```

To persist a run and compare against a previous one:

```bash
./build/benchmarks/bench --benchmark_format=json --benchmark_out=results/$(date +%Y%m%d_%H%M%S).json
python3 build/_deps/benchmark-src/tools/compare.py benchmarks results/<old>.json results/<new>.json
```
 

## Project Structure
 
```
include/
  ├── types.h          # All ITCH 5.0 message structs (packed POD), MessageType
  │                    # constants, compile-time length table, Message variant
  ├── parser.h         # ITCHReader + parser:: namespace (per-type decode functions)
  ├── lob.h            # Order, OrderbookT + HashMapBook/SortedVectorBook policies, OrderbookManager
  └── ring_buffer.h    # SPSCRingBuffer<N> — lock-free single-producer/single-consumer queue
 
src/
  ├── types.cpp        # Timestamp/stock helpers, ostream operators
  ├── parser.cpp       # ITCHReader::read_messages, parser:: implementations
  ├── lob.cpp          # OrderbookT, book policies, and OrderbookManager method implementations
  └── main.cpp         # Entry point - direct or ring buffer mode, timing output
 
tests/
  ├── parser_test.cpp  # Google Test: parser namespace + ITCHParser unit tests
  └── lob_test.cpp     # Google Test: OrderbookT and OrderbookManager unit tests (typed over both backends)
 
benchmarks/
  └── bench.cpp        # Google Benchmark targets
 
cmake/
  └── FetchDependencies.cmake  # FetchContent for GTest, Google Benchmark, unordered_dense
```
 

## Design
 
**Parser**
 
`ITCHReader` reads the binary file in 64 KB chunks (tunable) into a stack-allocated buffer. Leftover bytes at the end of each chunk are shifted to the front before the next read, avoiding message boundary splits. Each message is decoded via `memcpy` + byte-swapping (`beXXtoh`), which is safe on any alignment and correct for ITCH's big-endian format.
 
The message type byte dispatches via a `switch` to a dedicated `parser::parse_*` function. Unhandled types advance the cursor without allocation. A `std::variant<..., std::monostate>` (`Message`) carries the decoded result to the caller through a `std::function` callback, keeping `ITCHReader` fully decoupled from order book logic.
 
**Order Book**
 
`OrderbookT<BidBook, AskBook>` stores each side's price levels (price → aggregate quantity) behind a swappable book policy. Price is stored as the raw ITCH integer with 4 implied decimal places ($1.00 = 10000). Two policies are implemented:

- `HashMapBook` (`std::unordered_map<uint32_t, uint64_t>`) — O(1) average add/remove, but BBO requires an O(n) scan since entries are unordered.
- `SortedVectorBook<Compare>` (sorted `std::vector<PriceLevel>`, binary-searched) — O(log n) add/remove via `lower_bound` plus an O(n) `insert`/`erase` shift, but O(1) BBO since the best price sits at the front.

`FastOrderbook = OrderbookT<HashMapBook, HashMapBook>` and `BBOOrderbook = OrderbookT<SortedVectorBook<greater>, SortedVectorBook<less>>` (bids sorted high→low, asks low→high) are benchmarked side by side to compare the two trade-offs at varying book depths.

`OrderbookManager<OrderbookT_>` maintains two maps:
- `books`: `unordered_map<uint64_t, OrderbookT_>` - keyed by a 64-bit reinterpretation of the 8-byte ticker (`ticker_key`); one entry per traded symbol, populated once and rarely touched again, so the container choice matters little here
- `orders`: `ankerl::unordered_dense::map<uint64_t, Order>` - order reference number → order state, looked up on every execute/cancel/delete/replace. This map grows to millions of live entries with constant insert/erase churn, which is exactly where `unordered_dense` earns its keep: `std::unordered_map` is separate-chaining (each bucket a linked list of individually heap-allocated nodes), while `unordered_dense::map` is flat/open-addressing over a contiguous array — fewer allocations and far better cache locality on the hot lookup path

`std::visit` on the `Message` variant dispatches to the correct `handle` overload with no virtual dispatch.
 
**SPSC Ring Buffer**
 
`SPSCRingBuffer<N>` is a lock-free queue for decoupling the parser thread (producer) from the order book thread (consumer). Power-of-two capacity is enforced via `static_assert` so that index masking (`& (N-1)`) replaces modulo. `alignas(std::hardware_destructive_interference_size)` on `head`, `tail`, and the slot array prevents false sharing across cache lines. Acquire/release memory ordering on the index loads and stores provides the minimum synchronisation required for correctness without a full memory barrier.
 
Enabled at compile time via `-DUSE_RING_BUFFER`.

**Trade-offs**

Despite decoupling parsing and processing across threads, the ring buffer is slower than direct single-threaded execution at every dataset size tested — thread hand-off and synchronisation overhead outweigh the benefit of overlap, and direct processing keeps better cache locality.


## Message Handling
 
All 23 ITCH 5.0 message types are fully defined as packed structs with compile-time size assertions. The parser actively decodes and routes the 7 types that affect the order book:
 
| Type | Message | LOB Effect |
|------|---------|------------|
| `A` | Add Order | Insert quantity at price level |
| `F` | Add Order w/ MPID Attribution | Insert quantity at price level |
| `E` | Order Executed | Reduce quantity; remove order on full fill |
| `C` | Order Executed w/ Price | Reduce quantity; remove order on full fill |
| `X` | Order Cancel | Partial quantity reduction at price level |
| `D` | Order Delete | Full removal of order and price level cleanup |
| `U` | Order Replace | Atomic delete + re-add at new price/quantity |
 
All remaining types (`S`, `R`, `H`, `Y`, `L`, `V`, `W`, `K`, `J`, `h`, `P`, `Q`, `B`, `I`, `N`, `O`) are structurally defined for correctness and skipped during processing since they carry no order book state.
 
 
## Build
 
**Requirements**
 
- CMake $\geq$ 3.20
- C++23-capable compiler (GCC 13+, Clang 16+)
- Internet access on first build (FetchContent fetches GTest, Google Benchmark, unordered_dense automatically)

```bash
git clone https://github.com/cjramsey/NASDAQ-ITCH-LOB.git
cd NASDAQ-ITCH-LOB
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```
 
This produces four executables:
 
| Target | Mode | Flags |
|--------|------|-------|
| `direct` | Single-threaded | `-O3`, LTO |
| `ring_buffer` | SPSC producer-consumer | `-O3`, LTO |
| `direct_perf` | Single-threaded, profiler-friendly | `-O2 -g -fno-omit-frame-pointer` |
| `ring_buffer_perf` | SPSC, profiler-friendly | `-O2 -g -fno-omit-frame-pointer` |
 
To build without benchmarks:
 
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=OFF
```


## Usage
 
Download a TotalView-ITCH 5.0 sample file from NASDAQ:
 
```
https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/
```

Execute using direct mode:
 
```bash
./build/direct [file-path]
```

Execute using ring buffer mode:
 
```bash
./build/ring_buffer [file-path]
```
 
On exit, the program prints:
 
```
Messages: <count>
Time: <ms>
Throughput: <msg/s>
Efficiency: <ns/msg>
```


## Testing
 
Unit tests cover `OrderbookT`, `OrderbookManager` and `parser::` using Google Test. Orderbook/manager tests are typed over both `FastOrderbook` and `BBOOrderbook`: add bid/ask, partial cancel, full delete, partial and full execution, execution with price, and order replace, plus `BBOOrderbook`-specific tests for `best()` tracking. Ticker key conversion and timestamp parsing are also tested.
 
```bash
ctest --test-dir build --output-on-failure
```
 
 
## Roadmap
 
- [X] Investigate flat sorted price-level representation vs `unordered_map` at shallow book depths — implemented as `BBOOrderbook`, benchmarked side by side with the original `unordered_map`-backed `FastOrderbook`
- [ ] Top-of-book BBO output stream
- [ ] Persist L2 order book data over time, split into logical files by event type (adds/deletes/modifies/etc.) in Parquet/Arrow
- [ ] Data analysis on the persisted data in Python (polars)
- [ ] Reconstruct L3 order book data (full per-order detail, not just aggregated price levels)
 

## References
 
- [NASDAQ TotalView-ITCH 5.0 Specification](http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf)
- [NASDAQ Historical Data](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/)
- [ankerl::unordered_dense](https://github.com/martinus/unordered_dense)