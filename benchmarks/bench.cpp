#include <benchmark/benchmark.h>
#include <parser.h>
#include <lob.h>
#include <ring_buffer.h>
#include <filesystem>
#include <thread>

// Sets the standard set of throughput/latency counters shared by every
// benchmark below: messages/sec, ns/msg, and bytes/sec (native Benchmark
// column, driven off the input file size).
static void SetThroughputCounters(benchmark::State& state, const std::string& path) {
    state.counters["msg/s"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate);

    state.counters["ns/msg"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate |
                           benchmark::Counter::kInvert);

    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) *
                             static_cast<int64_t>(std::filesystem::file_size(path)));
}

// Parser benchmarks

static void BM_ParseOnly(benchmark::State& state, std::string path) {
    for (auto _ : state) {
        state.PauseTiming();
        uint64_t counter{};
        ITCHReader reader{path};
        state.ResumeTiming();

        reader.read_messages([](Message&& msg) {
            benchmark::DoNotOptimize(msg);
        }, counter);

        benchmark::DoNotOptimize(counter);
        state.counters["Messages"] += counter;
    }

    SetThroughputCounters(state, path);
}
BENCHMARK_CAPTURE(BM_ParseOnly, 10_million, std::string("./data/sample10million.NASDAQ_ITCH50"))
    ->Repetitions(5)->ReportAggregatesOnly(true);
BENCHMARK_CAPTURE(BM_ParseOnly, 100_million, std::string("./data/sample100million.NASDAQ_ITCH50"))
    ->Repetitions(5)->ReportAggregatesOnly(true);
BENCHMARK_CAPTURE(BM_ParseOnly, 1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5);


// Orderbook benchmarks

template <typename OrderbookT_>
static void BM_ParseAndUpdateLOB(benchmark::State& state, std::string path) {
    for (auto _ : state) {
        state.PauseTiming();
        ITCHReader reader{path};
        OrderbookManager<OrderbookT_> manager{};
        uint64_t counter{};
        state.ResumeTiming();

        reader.read_messages([&manager](Message&& msg) {
            manager.process(msg);
        }, counter);

        benchmark::DoNotOptimize(manager);
        benchmark::DoNotOptimize(counter);
        state.counters["Messages"] += counter;
    }

    SetThroughputCounters(state, path);
}
BENCHMARK_TEMPLATE1_CAPTURE(BM_ParseAndUpdateLOB, FastOrderbook, Fast_10_million, std::string("./data/sample10million.NASDAQ_ITCH50"))
    ->Repetitions(5)->ReportAggregatesOnly(true);
BENCHMARK_TEMPLATE1_CAPTURE(BM_ParseAndUpdateLOB, FastOrderbook, Fast_100_million, std::string("./data/sample100million.NASDAQ_ITCH50"))
    ->Repetitions(5)->ReportAggregatesOnly(true);
BENCHMARK_TEMPLATE1_CAPTURE(BM_ParseAndUpdateLOB, FastOrderbook, Fast_1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5);
BENCHMARK_TEMPLATE1_CAPTURE(BM_ParseAndUpdateLOB, BBOOrderbook, BBO_10_million, std::string("./data/sample10million.NASDAQ_ITCH50"))
    ->Repetitions(5)->ReportAggregatesOnly(true);
BENCHMARK_TEMPLATE1_CAPTURE(BM_ParseAndUpdateLOB, BBOOrderbook, BBO_100_million, std::string("./data/sample100million.NASDAQ_ITCH50"))
    ->Repetitions(5)->ReportAggregatesOnly(true);
BENCHMARK_TEMPLATE1_CAPTURE(BM_ParseAndUpdateLOB, BBOOrderbook, BBO_1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5);


template <std::size_t N, typename OrderbookT_>
static void BM_RingBuffer(benchmark::State& state, std::string path) {
    for (auto _ : state) {
        state.PauseTiming();
        ITCHReader reader{path};
        OrderbookManager<OrderbookT_> manager{};
        uint64_t counter{};

        SPSCRingBuffer<N> ring_buffer{};
        std::atomic<bool> running{true};

        auto ring_buffer_handler = [&ring_buffer](Message&& msg) {
            while (!ring_buffer.push(std::move(msg)));
        };

        state.ResumeTiming();

        std::thread t1{[&reader, &ring_buffer_handler, &counter, &running]() {
            reader.read_messages(ring_buffer_handler, counter);
            running.store(false, std::memory_order_release);
        }};

        std::thread t2{[&manager, &ring_buffer, &running](){
            Message msg;
            while (running.load(std::memory_order_acquire) || !ring_buffer.empty()) {
                if (ring_buffer.pop(msg)) {
                    manager.process(msg);
                }
            }
        }};

        benchmark::DoNotOptimize(manager);

        t1.join();
        t2.join();

        state.counters["Messages"] += counter;
    }

    SetThroughputCounters(state, path);
}
// UseRealTime(): the actual parse/process work happens on spawned threads,
// not the calling (benchmark) thread, so CPU time (Benchmark's default rate
// denominator) is nearly zero here — it would make msg/s, ns/msg, and
// bytes_per_second meaningless. Wall-clock time is what's real.
BENCHMARK_CAPTURE((BM_RingBuffer<1 << 12, FastOrderbook>), Fast_10_million, std::string("./data/sample10million.NASDAQ_ITCH50"))
    ->Iterations(100)->Repetitions(5)->ReportAggregatesOnly(true)->UseRealTime();
BENCHMARK_CAPTURE((BM_RingBuffer<1 << 12, FastOrderbook>), Fast_100_million, std::string("./data/sample100million.NASDAQ_ITCH50"))
    ->Iterations(100)->Repetitions(5)->ReportAggregatesOnly(true)->UseRealTime();
BENCHMARK_CAPTURE((BM_RingBuffer<1 << 12, FastOrderbook>), Fast_1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5)->UseRealTime();
BENCHMARK_CAPTURE((BM_RingBuffer<1 << 12, BBOOrderbook>), BBO_10_million, std::string("./data/sample10million.NASDAQ_ITCH50"))
    ->Iterations(100)->Repetitions(5)->ReportAggregatesOnly(true)->UseRealTime();
BENCHMARK_CAPTURE((BM_RingBuffer<1 << 12, BBOOrderbook>), BBO_100_million, std::string("./data/sample100million.NASDAQ_ITCH50"))
    ->Iterations(100)->Repetitions(5)->ReportAggregatesOnly(true)->UseRealTime();
BENCHMARK_CAPTURE((BM_RingBuffer<1 << 12, BBOOrderbook>), BBO_1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5)->UseRealTime();

BENCHMARK_MAIN();