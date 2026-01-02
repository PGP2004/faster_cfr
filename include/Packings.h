#pragma once
#include <cstdint>
#include <stdexcept>
#include <iostream>
#include <Profiler.h>

using std::invalid_argument;
using std::logic_error;
using std::cout;

struct PackedActions {
    uint64_t w = 0;
    int len = 0;

    static constexpr int BITS_PER_ACTION = 3;
    static constexpr int MAX_ACTIONS = 64 / BITS_PER_ACTION; // 21
    static constexpr uint64_t MASK = (1ULL << BITS_PER_ACTION) - 1ULL; // 0b111

    void push(int action_val) {
        PROFILE_FUNCTION();
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


    int get_or_empty(int idx) const {
        PROFILE_FUNCTION();
        if (idx < 0 || idx >= MAX_ACTIONS) throw logic_error("idx out of range");
        if (idx >= len) return 0;
        int shift = BITS_PER_ACTION * idx;
        return int((w >> shift) & MASK); // 0..7
    }

    int get(int idx) const {
        PROFILE_FUNCTION();
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

    // card must be 1..52. 0 is reserved for "empty".
    // If your natural IDs are 0..51, pass (card_id + 1).
    void push(int card_val) {
        PROFILE_FUNCTION();
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
        PROFILE_FUNCTION();
        if (idx < 0 || idx >= len) throw logic_error("idx out of range");
        int shift = BITS_PER_CARD * idx;
        int v = int((w >> shift) & MASK);
        if (v == 0) throw logic_error("packed card was empty (0) inside active prefix");
        return v; // 1..52
    }
};

