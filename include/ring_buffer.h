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
        const auto h = head.load(std::memory_order_relaxed);
        if (h - tail.load(std::memory_order_acquire) == N) {
            return false;
        }
        buffer[h & (N - 1)] = msg;
        head.store(h + 1, std::memory_order_release);
        return true;
    }

    bool pop(Message& msg) {
        const auto t = tail.load(std::memory_order_relaxed);
        if (head.load(std::memory_order_acquire) == t) {
            return false;
        }
        msg = std::move(buffer[t & (N - 1)]);
        tail.store(t + 1, std::memory_order_release);
        return true;
    }

    bool empty() {
        const auto t = tail.load(std::memory_order_relaxed);
        return head.load(std::memory_order_acquire) == t;
    }

    SPSCRingBuffer() = default;

    SPSCRingBuffer(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer& operator=(const SPSCRingBuffer&) = delete;
    SPSCRingBuffer(SPSCRingBuffer&&) = delete;
    SPSCRingBuffer& operator=(SPSCRingBuffer&&) = delete;
};

