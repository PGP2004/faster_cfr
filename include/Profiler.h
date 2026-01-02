#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

using std::string;
using std::unordered_map;
using std::chrono::steady_clock;
using std::chrono::duration;

// Global profiling data structure
struct ProfileStats {
    double total_ms = 0.0;          // Total time including child calls
    double exclusive_ms = 0.0;      // Time excluding child calls
    size_t call_count = 0;
};

class Profiler {
public:
    // Forward declare ScopedTimer
    class ScopedTimer;

private:
    static unordered_map<string, ProfileStats>& get_stats() {
        static unordered_map<string, ProfileStats> stats;
        return stats;
    }

    // Thread-local stack to track nested profiler calls
    static thread_local std::vector<ScopedTimer*> call_stack;

public:
    // RAII timer that records elapsed time on destruction
    class ScopedTimer {
    private:
        string func_name;
        steady_clock::time_point start;
        double child_time_ms;

    public:
        explicit ScopedTimer(const string& name)
            : func_name(name), start(steady_clock::now()), child_time_ms(0.0) {
            // Push this timer onto the call stack
            Profiler::call_stack.push_back(this);
        }

        ~ScopedTimer() {
            auto end = steady_clock::now();
            duration<double, std::milli> elapsed = end - start;
            double total_time = elapsed.count();
            double exclusive_time = total_time - child_time_ms;
            
            auto& stats = Profiler::get_stats();
            stats[func_name].total_ms += total_time;
            stats[func_name].exclusive_ms += exclusive_time;
            stats[func_name].call_count += 1;

            // Pop this timer from the call stack
            Profiler::call_stack.pop_back();

            // Add this time to the parent's child_time
            if (!Profiler::call_stack.empty()) {
                Profiler::call_stack.back()->child_time_ms += total_time;
            }
        }

        friend class Profiler;
    };

    // Print profiling results sorted by exclusive time
    static void print_results() {
        auto& stats = get_stats();
        
        if (stats.empty()) {
            std::cout << "\n=== No profiling data collected ===\n";
            return;
        }

        // Convert to vector for sorting by exclusive time
        std::vector<std::pair<string, ProfileStats>> sorted_stats(stats.begin(), stats.end());
        std::sort(sorted_stats.begin(), sorted_stats.end(),
                  [](const auto& a, const auto& b) {
                      return a.second.exclusive_ms > b.second.exclusive_ms;
                  });

        std::cout << "\n=== Profiling Results (sorted by exclusive time) ===\n";
        std::cout << std::fixed << std::setprecision(3);
        std::cout << std::left;
        std::cout << std::setw(35) << "Function"
                  << std::right
                  << std::setw(13) << "Excl (ms)"
                  << std::setw(13) << "Total (ms)"
                  << std::setw(11) << "Calls"
                  << std::setw(13) << "Avg (ms)"
                  << "\n";
        std::cout << std::string(85, '-') << "\n";

        double grand_total_exclusive = 0.0;
        for (const auto& [name, stat] : sorted_stats) {
            double avg = stat.exclusive_ms / static_cast<double>(stat.call_count);
            std::cout << std::left << std::setw(35) << name
                      << std::right
                      << std::setw(13) << stat.exclusive_ms
                      << std::setw(13) << stat.total_ms
                      << std::setw(11) << stat.call_count
                      << std::setw(13) << avg
                      << "\n";
            grand_total_exclusive += stat.exclusive_ms;
        }

        std::cout << std::string(85, '-') << "\n";
        std::cout << std::left << std::setw(35) << "TOTAL (Exclusive)"
                  << std::right << std::setw(13) << grand_total_exclusive << "\n";
        std::cout << "\nNote: 'Excl' = time in function itself (excluding child calls)\n";
        std::cout << "      'Total' = time including child function calls\n";
        std::cout << "\n";
    }

    // Reset all profiling data
    static void reset() {
        get_stats().clear();
    }
};

// Convenience macro to profile a function
// #define PROFILE_FUNCTION() Profiler::ScopedTimer _profiler_timer_(__FUNCTION__)
#define PROFILE_FUNCTION() {} // noop macro to avoid compiler errors
#define PROFILE_SCOPE(name) Profiler::ScopedTimer _profiler_timer_##__LINE__(name)

