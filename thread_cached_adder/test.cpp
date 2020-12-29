#include <cstdlib>
#include <iostream>
#include <omp.h>
#include <chrono>
#include "adder.h"

int main(int argc, char **argv) {
    concurrent::Adder<uint32_t> int32_adder;
    concurrent::Adder<uint64_t> int64_adder;
    std::atomic<uint32_t> int32_atomic(0);
    std::atomic<uint64_t> int64_atomic(0);

    uint32_t seed = 12345678;
    if(argc > 1) {
        seed = std::atoi(argv[1]);
    }
    std::srand(seed);
    uint32_t delta32 = std::rand() & 0xf;
    uint64_t delta64 = std::rand() & 0xffffff;
    int nthreads;
    #pragma omp parallel
    {
        #pragma omp single
        nthreads = omp_get_num_threads();
    }
    std::cout << "delta32:      " << delta32 << std::endl;
    std::cout << "delta64:      " << delta64 << std::endl;
    std::cout << "nthreads:     " << nthreads << std::endl << std::endl;

    auto beginTime = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    for (int i = 0; i < 1000000; ++i) {
        int32_adder.add(delta32);
        int64_adder.add(delta64);
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
    std::cout << "adder time:   " << elapsedTime.count() << " milliseconds" << std::endl;
    std::cout << "int32_adder:  " << int32_adder.sum() << std::endl;
    std::cout << "int64_adder:  " << int64_adder.sum() << std::endl << std::endl;
    
    beginTime = std::chrono::high_resolution_clock::now();
    #pragma omp parallel
    for (int i = 0; i < 1000000; ++i) {
        int32_atomic.fetch_add(delta32);
        int64_atomic.fetch_add(delta64);
    }
    endTime = std::chrono::high_resolution_clock::now();
    elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - beginTime);
    std::cout << "atomic time:  " << elapsedTime.count() << " milliseconds" << std::endl;
    std::cout << "int32_atomic: " << int32_atomic.load() << std::endl;
    std::cout << "int64_atomic: " << int64_atomic.load() << std::endl << std::endl;
}