
# NASDAQ ITCH 5.0 - Parser & Limit Order Book Reconstructor
 
A high-performance C++23 implementation of a NASDAQ TotalView-ITCH 5.0 binary feed parser with full limit order book (LOB) reconstruction across all tracked instruments. Built to explore low-latency feed handling and market microstructure on real exchange data.
 
---
 
## Overview
 
NASDAQ's TotalView-ITCH 5.0 protocol delivers the complete order-by-order history of the exchange as a raw binary stream: every add, cancel, delete, execution, and replace, for every security, across an entire trading day. This project parses that stream and reconstructs the per-symbol limit order book in real time, maintaining aggregated quantity at each price level.
 
Two execution modes are provided: a direct single-threaded mode and a producer-consumer mode via a lock-free SPSC ring buffer, allowing the parser and order book update logic to be decoupled across threads.
 
---

## Architecture
 
```
include/
  ├── types.h          # All ITCH 5.0 message structs (packed POD), MessageType
  │                    # constants, compile-time length table, Message variant
  ├── parser.h         # ITCHReader + parser:: namespace (per-type decode functions)
  ├── lob.h            # Order, Orderbook, OrderbookManager
  └── ring_buffer.h    # SPSCRingBuffer<N> — lock-free single-producer/single-consumer queue
 
src/
  ├── types.cpp        # Timestamp/stock helpers, ostream operators
  ├── parser.cpp       # ITCHReader::read_messages, parser:: implementations
  ├── lob.cpp          # Orderbook and OrderbookManager method implementations
  └── main.cpp         # Entry point - direct or ring buffer mode, timing output
 
tests/
  └── lob_test.cpp     # Google Test: Orderbook and OrderbookManager unit tests
 
benchmarks/
  └── bench.cpp        # Google Benchmark targets (in progress)
 
cmake/
  └── FetchDependencies.cmake  # FetchContent for GTest, Google Benchmark, unordered_dense
```
 
---

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

---

## Performance
 
Measured on a sample NASDAQ TotalView-ITCH 5.0 file on an Intel Core i7-1165G7 @ 2.80 GHz, 16 GB RAM, compiled with `-O3 -march=native` and link-time optimization enabled.
 
| Input Size | Approx. Latency/msg |
|---|---|
| 100 MB | ~25 ns |
| 1 GB | ~90 ns |
| Full day (~12 GB) | Degrades further |
 
The ~25 ns figure at 100 MB reflects a working set that fits in L3 cache. As input size grows, the order index and per-symbol price maps together exceed cache capacity. At that point the dominant cost becomes random-access main memory latency on order lookups for executes, cancels, and deletes, not decode logic.
 
> Formal benchmarking via Google Benchmark is in progress. Current figures are derived from `std::chrono::high_resolution_clock` wall-clock measurements printed at program exit.

---

## Design
 
**Parser**
 
`ITCHReader` reads the binary file in 64 KB chunks (tunable) into a stack-allocated buffer. Leftover bytes at the end of each chunk are shifted to the front before the next read, avoiding message boundary splits. Each message is decoded via `memcpy` + `beXXtoh` byte-swap, which is safe on any alignment and correct for ITCH's big-endian format.
 
The message type byte dispatches via a `switch` to a dedicated `parser::parse_*` function. Unhandled types advance the cursor without allocation. A `std::variant<..., std::monostate>` (`Message`) carries the decoded result to the caller through a `std::function` callback, keeping `ITCHReader` fully decoupled from order book logic.
 
**Order Book**
 
`Orderbook` stores each side as `std::unordered_map<uint32_t, uint64_t>` (price → aggregate quantity). Price is stored as the raw ITCH integer with 4 implied decimal places ($1.00 = 10000).
 
`OrderbookManager` maintains two maps:
- `books`: `unordered_map<uint64_t, Orderbook>` - keyed by a 64-bit reinterpretation of the 8-byte ticker (`ticker_key`)
- `orders`: `ankerl::unordered_dense::map<uint64_t, Order>` - order reference number $\rightarrow$ order state, used to look up price/side/symbol on execute, cancel, delete, and replace
`std::visit` on the `Message` variant dispatches to the correct `handle` overload with no virtual dispatch.
 
**SPSC Ring Buffer**
 
`SPSCRingBuffer<N>` is a lock-free queue for decoupling the parser thread (producer) from the order book thread (consumer). Power-of-two capacity is enforced via `static_assert` so that index masking (`& (N-1)`) replaces modulo. `alignas(std::hardware_destructive_interference_size)` on `head`, `tail`, and the slot array prevents false sharing across cache lines. Acquire/release memory ordering on the index loads and stores provides the minimum synchronisation required for correctness without a full memory barrier.
 
Enabled at compile time via `-DUSE_RING_BUFFER`.
 
---
 
## Build
 
**Requirements**
 
- CMake $\geq$ 3.20
- C++23-capable compiler (GCC 13+, Clang 16+)
- Internet access on first build (FetchContent fetches GTest, Google Benchmark, unordered_dense automatically)

```bash
git clone https://github.com/cjramsey/NASDAQ-ITCH-LOB.git
cd NASDAQ-ITCH-LOB
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
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
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_BENCHMARKS=OFF
```

---

## Usage
 
Download a TotalView-ITCH 5.0 sample file from NASDAQ:
 
```
https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/
```
 
Files are gzip-compressed; decompress before use:
 
```bash
gunzip 20190830.NASDAQ_ITCH50.gz
```
 
Update the file path in `src/main.cpp`, then run:
 
```bash
./build/direct
```
 
On exit, the program prints:
 
```
Messages: <count>
Time: <ms>
Throughput: <msg/s>
Efficiency: <ns/msg>
```

---

## Testing
 
Unit tests cover `Orderbook` and `OrderbookManager` using Google Test, including: add bid/ask, partial cancel, full delete, partial and full execution, execution with price, and order replace. Ticker key conversion and timestamp parsing are also tested.
 
```bash
cd build && ctest --output-on-failure
```
 
---
 
## Roadmap
 
- [ ] Complete Google Benchmark integration
- [ ] Configurable file path via CLI argument rather than hardcoded path
- [ ] Arena/pool allocator for `Order` objects to improve spatial locality at scale
- [ ] Investigate flat sorted price-level representation vs `unordered_map` at shallow book depths
- [ ] Parser unit tests
- [ ] Top-of-book BBO output stream
 
---

## References
 
- [NASDAQ TotalView-ITCH 5.0 Specification](http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf)
- [NASDAQ Historical Data](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/)
- [ankerl::unordered_dense](https://github.com/martinus/unordered_dense)

---