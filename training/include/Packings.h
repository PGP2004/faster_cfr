#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>
#include <string>
#include "utils.h"

struct PackedActions {
    uint64_t w = 0;
    int len = 0;

    static constexpr int BITS_PER_ACTION = 3;
    static constexpr int MAX_ACTIONS = 64 / BITS_PER_ACTION; // 21
    static constexpr uint64_t MASK = (1ULL << BITS_PER_ACTION) - 1ULL; // 0b111

    void push(int action_val) {
        if (action_val < 0 || action_val > 6) throw std::invalid_argument("action_val must be in [0,6]");
        if (len + 1 > MAX_ACTIONS) throw std::logic_error("Cannot pack any more actions");

        int shift = BITS_PER_ACTION * len;
        uint64_t slot_mask = MASK << shift;
        w = (w & ~slot_mask) | ((uint64_t(action_val + 1) & MASK) << shift);
        ++len;
    }

    int get(int idx) const {
        if (idx < 0 || idx >= len) throw std::logic_error("idx out of range");
        int shift = BITS_PER_ACTION * idx;
        int v = int((w >> shift) & MASK);
        if (v == 0)  throw std::logic_error("packed action was empty (0)");
        return v - 1;
    }

    void del() {
        if (len <= 0) throw std::logic_error("del from empty PackedActions");
        int shift = BITS_PER_ACTION * (len - 1);
        w &= ~(MASK << shift);
        --len;
    }

    bool operator==(const PackedActions& other) const noexcept {
        return w == other.w && len == other.len;
    }
};


//TODO: Add the clustering results to the infokey // and j rework this class hella
struct InfoKey {
    int street;
    int hand_id;
    PackedActions packed_actions;

bool operator==(const InfoKey&) const = default;
};

struct InfoKeyHash{
    // kinda inspired by hash func used by benq (this one guy whos really smart)
    const uint64_t C = uint64_t(2e18*(3.14))+71;
    size_t operator()(const InfoKey& k) const {
    uint64_t combined = (k.packed_actions.w + 0x9e3779b97f4a7c15ULL)
                      ^ (uint64_t(k.hand_id) * 0x517cc1b727220a95ULL)
                      ^ (uint64_t(k.street)  * 0x2545F4914F6CDD1DULL);
    return __builtin_bswap64(combined * C);
    }

};


//Bobby black magic

struct Abstraction {
    
    std::vector<uint8_t> preflop_clusters;
    std::vector<uint16_t> flop_clusters;
    std::vector<uint16_t> turn_clusters;
    std::vector<uint8_t> river_clusters;

    Abstraction() = default;  // keep default-constructible

    Abstraction(const std::string& flop_path,
                const std::string& turn_path,
                const std::string& river_path) {
        set_clusters(flop_path, turn_path, river_path);
    }

    void set_clusters(const std::string& flop_path, const std::string&turn_path, const std::string& river_path){
        preflop_clusters.resize(169);

        for (size_t i = 0; i < 169; ++i){
            preflop_clusters[i] = static_cast<int>(i);
        }

        auto [fc, flop_header] = load_matrix_and_header<uint16_t>(flop_path);
        flop_clusters = std::move(fc);

        auto[tc, turn_header] = load_matrix_and_header<uint16_t>(turn_path);
        turn_clusters = std::move(tc);

        auto [rc, river_header] = load_matrix_and_header<uint8_t>(river_path);
        river_clusters = std::move(rc);

    }

    int cluster_of(int street, int hand_id) const {
        if (street == 0) return preflop_clusters[hand_id];
        if (street == 1) return flop_clusters[hand_id];
        if (street == 2) return turn_clusters[hand_id];
        if (street == 3) return river_clusters[hand_id];
        throw std::runtime_error("Street does not exist");
    }
};

