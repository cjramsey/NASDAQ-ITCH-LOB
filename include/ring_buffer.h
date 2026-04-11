#include <atomic>
#include <bit>
#include "types.h"

template<std::size_t N>
class SPSCRingBuffer
{
    static_assert(std::has_single_bit(N), "N must be a power of two.");

    using Slot = Message;

    alignas(std::hardware_destructive_interference_size)
    std::atomic<size_t> head{0};

    alignas(std::hardware_destructive_interference_size)
    std::atomic<size_t> tail{0};

    alignas(std::hardware_destructive_interference_size)
    Slot buffer[N];

public:
    template<typename U>
    requires std::convertible_to<U, Message>
    bool push(U&& msg) {
        // Producer is only thread which modifies head 
        // so memory order can be relaxed (still consistent ordering within same thread)
        const auto h = head.load(std::memory_order_relaxed);

        // Load of tail must be synced with stores from consumer thread
        if (h - tail.load(std::memory_order_acquire) == N) {
            return false;
        }

        buffer[h & (N - 1)] = msg;  // Bitwise-and over modulo, requires N to be power of two

        // Store to head must sync with loads from consumer thread 
        head.store(h + 1, std::memory_order_release);
        return true;
    }

    template<typename U>
    requires std::convertible_to<U, Message>
    bool pop(U& msg) {
        // Consumer is only thread which modifies tail
        // so memory order can be relaxed (still consistent ordering within same thread)
        const auto t = tail.load(std::memory_order_relaxed);

        // Load of head must be sycned with stores from producer thread
        if (head.load(std::memory_order_acquire) == t) {
            return false;
        }

        msg = std::move(buffer[t & (N - 1)]);

        // Store to tail must sync with loads from producer thread
        tail.store(t + 1, std::memory_order_release);
        return true;
    }

    bool empty() {
        // Method should only be called by consumer thread so relaxed memory order on tail load is appropriate
        const auto t = tail.load(std::memory_order_relaxed);

        // We must sync load of head with stores from the producer
        return head.load(std::memory_order_acquire) == t;
    }

    SPSCRingBuffer() = default;

    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer(SPSCRingBuffer&&) = delete;
    SPSCRingBuffer& operator=(SPSCRingBuffer&&) = delete;
};

