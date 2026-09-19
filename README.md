
# NASDAQ ITCH 5.0 - Parser & Limit Order Book Reconstructor
 
A C++23 NASDAQ TotalView-ITCH 5.0 binary feed parser with full limit order book (LOB) reconstruction across all tracked instruments — built to explore low-latency feed handling, cache-conscious data structures, and market microstructure on real exchange data.


## Key Highlights

- End-to-end NASDAQ ITCH 5.0 parser and limit order book reconstruction on real exchange data  
- ~9.4 ns/message decode-only throughput on 100MB; 320 ns/message full parse + order-book reconstruction across a real ~13GB, 423M-message trading day  
- Explores trade-offs between direct processing and lock-free SPSC queue architectures  
- Designed with attention to memory layout, cache behaviour, and minimal allocation  
- Reconstructed book exported to Parquet and used to replicate a published market-microstructure result (Cont, Kukanov & Stoikov) in Python  

 
## Contents

1. [Overview](#overview)
2. [Performance](#performance)
3. [Project Structure](#project-structure)
4. [Design](#design)
5. [Message Handling](#message--handling)
6. [Build](#build)
7. [Usage](#usage)
8. [Parquet Export (optional)](#parquet-export-optional)
9. [Analysis: Order Flow Imbalance and Price Impact](#analysis-order-flow-imbalance-and-price-impact)
10. [Testing](#testing)
11. [Roadmap](#roadmap)
12. [References](#references)

 
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
  ├── main.cpp         # Entry point - direct or ring buffer mode, timing output
  └── parquet_export/  # Optional itch_to_parquet tool: per-message-type writers
                       # (parquet_writer) + top-N depth snapshots (depth_writer)
 
tests/
  ├── parser_test.cpp  # Google Test: parser namespace + ITCHParser unit tests
  └── lob_test.cpp     # Google Test: OrderbookT and OrderbookManager unit tests (typed over both backends)
 
benchmarks/
  └── bench.cpp        # Google Benchmark targets

analysis/
  ├── src/analysis/ofi.py     # Shared pipeline: depth.parquet loading, OFI construction,
  │                           # interval aggregation, price-impact and per-window fits
  ├── ofi_analysis.ipynb      # Linear price impact of order flow imbalance
  ├── depth-analysis.ipynb    # Price impact vs market depth (beta = c / AD^lambda)
  └── pyproject.toml          # uv-managed environment (polars, statsmodels, plotly)
 
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


## Parquet Export (optional)

A separate `itch_to_parquet` tool decodes an ITCH file and writes one Parquet
file per message type — add/execute/cancel/delete/replace, plus trades — for
 analysis (e.g. with polars). It also reconstructs the book (`BBOOrderbook`) as it
goes and writes `depth.parquet`: one row per book-changing event with the top N
price levels per side as flat columns (`bid_px_00`, `bid_sz_00`, …, `ask_px_00`, …),
nulls padding levels deeper than the book. It requires Arrow/Parquet to already be installed.

**Install Arrow/Parquet (one-time):**

```bash
curl -fsSLo /tmp/arrow.deb "https://packages.apache.org/artifactory/arrow/ubuntu/apache-arrow-apt-source-latest-$(lsb_release -cs).deb" && sudo apt install -y -V /tmp/arrow.deb
sudo apt update && sudo apt install -y -V libarrow-dev libparquet-dev
```

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_PARQUET_EXPORT=ON
cmake --build build --target itch_to_parquet --parallel
```

```bash
./build/itch_to_parquet [input-file] [output-dir] [levels]
```

`[levels]` is the number of depth levels per side in `depth.parquet` (default 1, i.e. BBO only). Existing files in `[output-dir]` are overwritten. Produces:

```
add_order.parquet          order_cancel.parquet   order_replace.parquet
add_order_mpid.parquet     order_delete.parquet   trade.parquet
order_executed.parquet     cross_trade.parquet    broken_trade.parquet
order_executed_price.parquet                      depth.parquet
```


## Analysis: Order Flow Imbalance and Price Impact

Two notebooks in `analysis/` use the exported book to replicate Cont, Kukanov & Stoikov's
results on a single NASDAQ ITCH sample trading day. The input is `depth.parquet` at `levels=1`,
so every row is a change in the price or size of the best bid or ask, which is exactly the
event clock the paper's order flow imbalance measure is defined on. Data handling is polars
(the depth file is scanned lazily and collected with the streaming engine, since it runs to
hundreds of millions of rows), regressions are statsmodels, plots are plotly. Shared pipeline
code lives in `analysis/src/analysis/ofi.py` so both notebooks estimate from one definition.

**Setup**

```bash
cd analysis
uv sync
uv run jupyter lab
```

**Order flow imbalance (`ofi_analysis.ipynb`)**

For each book event $n$, the paper's order flow imbalance contribution is

$$e_n = \mathbb{1}_{\{P^b_n \geq P^b_{n-1}\}} q^b_n - \mathbb{1}_{\{P^b_n \leq P^b_{n-1}\}} q^b_{n-1} - \mathbb{1}_{\{P^a_n \leq P^a_{n-1}\}} q^a_n + \mathbb{1}_{\{P^a_n \geq P^a_{n-1}\}} q^a_{n-1}$$

where $P^b, q^b$ are the best bid price and size and $P^a, q^a$ the best ask. Summing $e_n$
over 10-second intervals of regular trading hours gives $\mathrm{OFI}_k$, regressed against the
mid-price change in ticks: $\Delta P_k = \alpha + \beta\,\mathrm{OFI}_k + \varepsilon_k$.

| Ticker | Events | $\beta$ (ticks/share) | $t(\beta)$ | $R^2$ |
|---|---|---|---|---|
| SPY | 4,161,382 | 1.41e-4 | 23.7 | 0.334 |
| QQQ | 4,207,542 | 1.94e-4 | 24.6 | 0.609 |
| AMD | 2,341,809 | 1.41e-4 | 39.9 | 0.789 |
| INTC | 1,586,139 | 2.28e-4 | 5.7 | 0.635 |
| GOOGL | 1,487,675 | 9.40e-3 | 10.5 | 0.138 |

Every $\beta$ is positive and strongly significant, and the binned means fall on a straight
line through the origin, so the sign and linearity of the relation reproduce cleanly. Fit
quality separates the names: AMD, QQQ and INTC land in the paper's reported $R^2$ range, SPY's
$\beta$ is robust but its $R^2$ sits below it, and GOOGL is the informative failure. At a
64-tick average spread the mid-price moves in jumps that top-of-book flow does not explain, and
its $R^2$ collapses accordingly. The interval length is a free parameter, and a sweep over
1s, 5s, 10s, 30s and 60s leaves every $t(\beta)$ above 4.8, so the result is not an artifact
of the paper's 10s choice.

**Depth scaling (`depth-analysis.ipynb`)**

The paper's structural explanation for why $\beta$ varies across stocks and across the day is
that price impact is inversely proportional to the depth available at the best quotes:

$$\beta_{i,w} = \frac{c}{AD_{i,w}^{\lambda}}, \qquad \lambda \approx 1$$

Following the paper's two-step procedure, $\beta_{i,w}$ is estimated per ticker per half-hour
window for the 25 most active tickers, $\hat\lambda$ comes from a log-log regression of $\beta$
on average depth, and $\hat{c}$ from a levels regression of $\beta$ on $AD^{-\hat\lambda}$.
After filtering to windows with at least 100 intervals and a significant $\beta$, 322 of 325
ticker-windows remain.

| Estimator | $\hat\lambda$ | 95% CI | $R^2$ | n |
|---|---|---|---|---|
| Pooled, common $c$ | 0.998 | (0.935, 1.062) | 0.966 | 322 |
| Fixed effects, per-ticker $c$ | 0.914 | (0.795, 1.033) | 0.594 | 322 |
| Median per-ticker | 0.972 | | | 25 |

$\lambda = 1$ falls inside the 95% interval for 20 of the 25 tickers individually, and the
pooled estimate is statistically indistinguishable from 1, matching the paper's grand mean of
0.98. Imposing $\lambda = 1$ gives a median $\hat{c}$ of 0.26 ticks against the paper's 0.45
and the stylized model's 0.5. The same mechanism explains the intraday pattern: $\beta$ opens
at 1.43 times its daily average while depth opens at 0.79, and the close reverses both.

**Inference**

All reported intervals are Wald tests on robust covariance estimates, which statsmodels
evaluates against the normal distribution rather than Student's t. Window-level and full-day
$\beta$ use White (HC1) errors, as the paper does. The per-ticker scaling regressions use
Newey-West, since the half-hour window series are autocorrelated. The pooled and fixed-effects
panel estimates cluster by ticker, without which treating 322 ticker-windows as independent
narrows the confidence interval by roughly a factor of three.

**Caveats**

One trading day, one venue (NASDAQ only, so the consolidated depth a real trader sees is
deeper than what is measured here), and $\beta$ estimated from 10-second intervals inside
30-minute windows. The paper uses 50 stocks over several weeks of NYSE TAQ data.


## Testing
 
Unit tests cover `OrderbookT`, `OrderbookManager` and `parser::` using Google Test. Orderbook/manager tests are typed over both `FastOrderbook` and `BBOOrderbook`: add bid/ask, partial cancel, full delete, partial and full execution, execution with price, and order replace, plus `BBOOrderbook`-specific tests for `best()` tracking. Ticker key conversion and timestamp parsing are also tested.
 
```bash
ctest --test-dir build --output-on-failure
```
 
 
## Roadmap
 
- [X] Investigate flat sorted price-level representation vs `unordered_map` at shallow book depths — implemented as `BBOOrderbook`, benchmarked side by side with the original `unordered_map`-backed `FastOrderbook`
- [X] Top-of-book BBO output stream — `depth.parquet` from the Parquet export tool (top-N levels per side, N=1 for BBO)
- [X] Persist L2 order book data over time, split into logical files by event type (adds/deletes/modifies/etc.) in Parquet/Arrow — see [Parquet Export](#parquet-export-optional)
- [X] Data analysis on the persisted data in Python (polars) — order flow imbalance and depth-scaling replication, see [Analysis](#analysis-order-flow-imbalance-and-price-impact)
- [ ] Multi-level order flow imbalance, requiring a ticker filter on the exporter so deeper books can be written without exporting every symbol
- [ ] Reconstruct L3 order book data (full per-order detail, not just aggregated price levels)
 

## References
 
- [NASDAQ TotalView-ITCH 5.0 Specification](http://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/NQTVITCHspecification.pdf)
- [NASDAQ Historical Data](https://emi.nasdaq.com/ITCH/Nasdaq%20ITCH/)
- [ankerl::unordered_dense](https://github.com/martinus/unordered_dense)
- Cont, R., Kukanov, A. and Stoikov, S. (2014). [The price impact of order book events](https://doi.org/10.1093/jjfinec/nbt003). *Journal of Financial Econometrics*, 12(1), 47–88. Working paper version (March 2011) at [arXiv:1011.6402](https://arxiv.org/abs/1011.6402), a copy of which is in `analysis/`