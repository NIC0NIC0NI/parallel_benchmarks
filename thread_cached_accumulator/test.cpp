#include <random>
#include <iostream>
#include "accumulator.h"
#include "../thread_test_utils.h"

constexpr std::plus<void> plus;

inline uint32_t get_seed(int argc, char **argv) {
    uint32_t seed = 12345678;
    if(argc > 1) {
        seed = std::atoi(argv[1]);
    }
    return seed;
}

int main(int argc, char **argv) {
    concurrent::Accumulator<uint32_t> int32_accumulator;
    concurrent::Accumulator<uint64_t> int64_accumulator;
    std::atomic<uint32_t> int32_atomic(0);
    std::atomic<uint64_t> int64_atomic(0);
    uint32_t seed = get_seed(argc, argv);
    std::mt19937_64 gen(seed);
    std::uniform_int_distribution<uint64_t> r(0, 0xfffffff);
    uint32_t delta32 = static_cast<uint32_t>(r(gen) & 0xff);
    uint64_t delta64 = r(gen);
    int nthreads = get_num_threads();
    std::cout << "delta32:           " << delta32 << std::endl;
    std::cout << "delta64:           " << delta64 << std::endl;
    std::cout << "nthreads:          " << nthreads << std::endl << std::endl;

    auto beginTime = std::chrono::high_resolution_clock::now();
    parallel([&int32_accumulator, &int64_accumulator, delta32, delta64] {
        auto& int32_instance = int32_accumulator.get_instance();
        auto& int64_instance = int64_accumulator.get_instance();
        for (int i = 0; i < 1000000; ++i) {
            int32_instance.accumulate(delta32, plus);
            int64_instance.accumulate(delta64, plus);
        }
    });
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
    std::cout << "accumulator time:  " << elapsedTime.count() << " milliseconds" << std::endl;
    std::cout << "int32_accumulator: " << int32_accumulator.result(plus) << std::endl;
    std::cout << "int64_accumulator: " << int64_accumulator.result(plus) << std::endl << std::endl;
    
    beginTime = std::chrono::high_resolution_clock::now();
    parallel([&int32_atomic, &int64_atomic, delta32, delta64] {
        for (int i = 0; i < 1000000; ++i) {
            int32_atomic.fetch_add(delta32);
            int64_atomic.fetch_add(delta64);
        }
    });
    endTime = std::chrono::high_resolution_clock::now();
    elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
    std::cout << "atomic time:       " << elapsedTime.count() << " milliseconds" << std::endl;
    std::cout << "int32_atomic:      " << int32_atomic.load() << std::endl;
    std::cout << "int64_atomic:      " << int64_atomic.load() << std::endl << std::endl;
}