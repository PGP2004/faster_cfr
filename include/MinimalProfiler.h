#pragma once
#include <chrono>
#include <string>
#include <iostream>
#include <iomanip>
#include <atomic>

using std::chrono::steady_clock;
using std::chrono::duration;

// Lightweight profiler - just tracks totals, no per-call overhead
struct MinimalProfiler {
    static std::atomic<uint64_t> hash_lookup_ns;
    static std::atomic<uint64_t> hash_insert_ns;
    static std::atomic<uint64_t> make_actions_ns;
    static std::atomic<uint64_t> infoset_ops_ns;
    static std::atomic<uint64_t> state_ops_ns;
    static std::atomic<uint64_t> rewards_ns;
    
    static std::atomic<uint64_t> hash_lookup_count;
    static std::atomic<uint64_t> hash_insert_count;
    
    static void print_results() {
        std::cout << "\n=== Minimal Profiling Results ===\n";
        std::cout << std::fixed << std::setprecision(3);
        
        double hash_lookup_ms = hash_lookup_ns.load() / 1e6;
        double hash_insert_ms = hash_insert_ns.load() / 1e6;
        double make_actions_ms = make_actions_ns.load() / 1e6;
        double infoset_ops_ms = infoset_ops_ns.load() / 1e6;
        double state_ops_ms = state_ops_ns.load() / 1e6;
        double rewards_ms = rewards_ns.load() / 1e6;
        
        std::cout << "Hash lookup:    " << std::setw(10) << hash_lookup_ms << " ms  (" 
                  << hash_lookup_count.load() << " calls, avg " 
                  << (hash_lookup_ms / hash_lookup_count.load() * 1000) << " µs)\n";
        std::cout << "Hash insert:    " << std::setw(10) << hash_insert_ms << " ms  (" 
                  << hash_insert_count.load() << " calls, avg " 
                  << (hash_insert_ms / hash_insert_count.load() * 1000) << " µs)\n";
        std::cout << "Make actions:   " << std::setw(10) << make_actions_ms << " ms\n";
        std::cout << "InfoSet ops:    " << std::setw(10) << infoset_ops_ms << " ms\n";
        std::cout << "State ops:      " << std::setw(10) << state_ops_ms << " ms\n";
        std::cout << "Get rewards:    " << std::setw(10) << rewards_ms << " ms\n";
        std::cout << "Total measured: " << std::setw(10) 
                  << (hash_lookup_ms + hash_insert_ms + make_actions_ms + infoset_ops_ms + state_ops_ms + rewards_ms) << " ms\n";
    }
    
    static void reset() {
        hash_lookup_ns = 0;
        hash_insert_ns = 0;
        make_actions_ns = 0;
        infoset_ops_ns = 0;
        state_ops_ns = 0;
        rewards_ns = 0;
        hash_lookup_count = 0;
        hash_insert_count = 0;
    }
};

// RAII timer for specific code blocks
struct ScopedMinimalTimer {
    std::atomic<uint64_t>& target;
    steady_clock::time_point start;
    
    explicit ScopedMinimalTimer(std::atomic<uint64_t>& t) 
        : target(t), start(steady_clock::now()) {}
    
    ~ScopedMinimalTimer() {
        auto end = steady_clock::now();
        auto elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        target.fetch_add(elapsed_ns, std::memory_order_relaxed);
    }
};

// #define TIME_HASH_LOOKUP() ScopedMinimalTimer _timer_(MinimalProfiler::hash_lookup_ns); MinimalProfiler::hash_lookup_count.fetch_add(1, std::memory_order_relaxed)
// #define TIME_HASH_INSERT() ScopedMinimalTimer _timer_(MinimalProfiler::hash_insert_ns); MinimalProfiler::hash_insert_count.fetch_add(1, std::memory_order_relaxed)
// #define TIME_MAKE_ACTIONS() ScopedMinimalTimer _timer_(MinimalProfiler::make_actions_ns)
// #define TIME_INFOSET_OPS() ScopedMinimalTimer _timer_(MinimalProfiler::infoset_ops_ns)
// #define TIME_STATE_OPS() ScopedMinimalTimer _timer_(MinimalProfiler::state_ops_ns)
// #define TIME_REWARDS() ScopedMinimalTimer _timer_(MinimalProfiler::rewards_ns)
#define TIME_HASH_LOOKUP() {}
#define TIME_HASH_INSERT() {}
#define TIME_MAKE_ACTIONS() {}
#define TIME_INFOSET_OPS() {}
#define TIME_STATE_OPS() {}
#define TIME_REWARDS() {}

