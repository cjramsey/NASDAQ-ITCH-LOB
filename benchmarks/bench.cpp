#include <benchmark/benchmark.h>
#include <parser.h>
#include <lob.h>
#include <ring_buffer.h>
#include <thread>

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

    // Throughput: messages per second
    state.counters["msg/s"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate);

    // Latency: nanoseconds per message
    state.counters["ns/msg"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate |
                           benchmark::Counter::kInvert);

}
BENCHMARK_CAPTURE(BM_ParseOnly, 10_million, std::string("./data/sample10million.NASDAQ_ITCH50"));
BENCHMARK_CAPTURE(BM_ParseOnly, 100_million, std::string("./data/sample100million.NASDAQ_ITCH50"));
BENCHMARK_CAPTURE(BM_ParseOnly, 1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5);


// Orderbook benchmarks

static void BM_ParseAndUpdateLOB(benchmark::State& state, std::string path) {
    for (auto _ : state) {
        state.PauseTiming();
        ITCHReader reader{path};
        OrderbookManager manager{};
        uint64_t counter{};
        state.ResumeTiming();

        reader.read_messages([&manager](Message&& msg) {
            manager.process(msg);
        }, counter);

        benchmark::DoNotOptimize(manager);
        benchmark::DoNotOptimize(counter);
        state.counters["Messages"] += counter;
    }

    state.counters["msg/s"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate);

    state.counters["ns/msg"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate |
                           benchmark::Counter::kInvert);
}
BENCHMARK_CAPTURE(BM_ParseAndUpdateLOB, 10_million, std::string("./data/sample10million.NASDAQ_ITCH50"));
BENCHMARK_CAPTURE(BM_ParseAndUpdateLOB, 100_million, std::string("./data/sample100million.NASDAQ_ITCH50"));
BENCHMARK_CAPTURE(BM_ParseAndUpdateLOB, 1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5);


template <std::size_t N>
static void BM_RingBuffer(benchmark::State& state, std::string path) {
    for (auto _ : state) {
        state.PauseTiming();
        ITCHReader reader{path};
        OrderbookManager manager{};
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
    state.counters["msg/s"] =
        benchmark::Counter(state.counters["Messages"],
                           benchmark::Counter::kIsRate);
    
    state.counters["ns/msg"] =
        benchmark::Counter(state.counters["Messages"],
                        benchmark::Counter::kIsRate |
                        benchmark::Counter::kInvert);
}
BENCHMARK_CAPTURE(BM_RingBuffer<1 << 12>, 10_million, std::string("./data/sample10million.NASDAQ_ITCH50"))->Iterations(100);
BENCHMARK_CAPTURE(BM_RingBuffer<1 << 12>, 100_million, std::string("./data/sample100million.NASDAQ_ITCH50"))->Iterations(100);
BENCHMARK_CAPTURE(BM_RingBuffer<1 << 12>, 1_billion, std::string("./data/sample1billion.NASDAQ_ITCH50"))->Iterations(5);

BENCHMARK_MAIN();