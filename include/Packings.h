#pragma once
#include <cstdint>
#include <stdexcept>
#include <iostream>

using std::invalid_argument;
using std::logic_error;
using std::pair;

struct PackedActions {
    uint64_t w = 0;
    int len = 0;

    static constexpr int BITS_PER_ACTION = 3;
    static constexpr int MAX_ACTIONS = 64 / BITS_PER_ACTION; // 21
    static constexpr uint64_t MASK = (1ULL << BITS_PER_ACTION) - 1ULL; // 0b111

    void push(int action_val) {

        if (action_val < 0 || action_val > 6) {
            throw invalid_argument("action_val must be in [0,6] (0 reserved for empty)");
        }


        if (len + 1 > MAX_ACTIONS) {
            throw logic_error("Cannot pack any more actions");
        }

        int shift = BITS_PER_ACTION * len;
        uint64_t slot_mask = MASK << shift;

        w = (w & ~slot_mask) | ((uint64_t(action_val+1) & MASK) << shift);
        ++len;
    }

    int get(int idx) const {
        if (idx < 0 || idx >= len) throw logic_error("idx out of range");
        int shift = BITS_PER_ACTION * idx;
        int v = int((w >> shift) & MASK);
        if (v == 0) throw logic_error("packed action was empty (0) inside active prefix");
        return v; 
    }

};

struct PackedCards {
    uint64_t w = 0;
    int len = 0;

    static constexpr int BITS_PER_CARD = 6;
    static constexpr int MAX_CARDS = 64 / BITS_PER_CARD; // 10
    static constexpr uint64_t MASK = (1ULL << BITS_PER_CARD) - 1ULL; // 0x3F

    void push(int card_val) {
        if (card_val < 0 || card_val > 52) {
            throw invalid_argument("card_val must be in [1,52] (0 reserved for empty)");
        }
        if (len + 1 > MAX_CARDS) {
            throw logic_error("Cannot pack any more cards");
        }

        int shift = BITS_PER_CARD * len;
        uint64_t slot_mask = MASK << shift;

        w = (w & ~slot_mask) | ((uint64_t(card_val+1) & MASK) << shift);
        ++len;
    }

    int get(int idx) const {
        if (idx < 0 || idx >= len) throw logic_error("idx out of range");
        int shift = BITS_PER_CARD * idx;
        int v = int((w >> shift) & MASK);
        if (v == 0) throw logic_error("packed card was empty (0) inside active prefix");
        return v; 
    }
};

using InfoKey = pair<uint64_t, uint64_t>;

struct InfoKeyHash {
    
    size_t operator()(const InfoKey& k) const noexcept {
        uint64_t x = k.first ^ (k.second + 0x9e3779b97f4a7c15ULL);

        x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ULL;
        x ^= x >> 27; x *= 0x94d049bb133111ebULL;
        x ^= x >> 31;

        return static_cast<size_t>(x);
    }
};