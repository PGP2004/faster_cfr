#include "MinimalProfiler.h"

std::atomic<uint64_t> MinimalProfiler::hash_lookup_ns{0};
std::atomic<uint64_t> MinimalProfiler::hash_insert_ns{0};
std::atomic<uint64_t> MinimalProfiler::make_actions_ns{0};
std::atomic<uint64_t> MinimalProfiler::infoset_ops_ns{0};
std::atomic<uint64_t> MinimalProfiler::state_ops_ns{0};
std::atomic<uint64_t> MinimalProfiler::rewards_ns{0};

std::atomic<uint64_t> MinimalProfiler::hash_lookup_count{0};
std::atomic<uint64_t> MinimalProfiler::hash_insert_count{0};

