#pragma once
#include <cstdint>
#include <stdexcept>

using std::invalid_argument;
using std::logic_error;

struct PackedActions {
    uint64_t w = 0;
    int len = 0;

    static constexpr int BITS_PER_ACTION = 3;
    static constexpr int MAX_ACTIONS = 64 / BITS_PER_ACTION; // 21
    static constexpr uint64_t MASK = (1ULL << BITS_PER_ACTION) - 1ULL; // 0b111

    void push(int action_val) {
        if (action_val < 0 || action_val > 6) throw invalid_argument("action_val must be in [0,6]");
        if (len + 1 > MAX_ACTIONS) throw logic_error("Cannot pack any more actions");

        int shift = BITS_PER_ACTION * len;
        uint64_t slot_mask = MASK << shift;
        w = (w & ~slot_mask) | ((uint64_t(action_val + 1) & MASK) << shift);
        ++len;
    }

    int get(int idx) const {
        if (idx < 0 || idx >= len) throw logic_error("idx out of range");
        int shift = BITS_PER_ACTION * idx;
        int v = int((w >> shift) & MASK);
        if (v == 0)  throw logic_error("packed action was empty (0)");
        return v - 1;
    }

    void del() {
        if (len <= 0) throw logic_error("del from empty PackedActions");
        int shift = BITS_PER_ACTION * (len - 1);
        w &= ~(MASK << shift);
        --len;
    }

    bool operator==(const PackedActions& other) const noexcept {
        return w == other.w && len == other.len;
    }
};

struct PackedCards {
    uint64_t w = 0;
    int len = 0;

    static constexpr int BITS_PER_CARD = 6;
    static constexpr int MAX_CARDS = 64 / BITS_PER_CARD; 
    static constexpr uint64_t MASK = (1ULL << BITS_PER_CARD) - 1ULL; // 0x3F

    void push(int card_val) {
        if (card_val < 0 || card_val > 51) throw invalid_argument("card_val must be in [0,51]");
        if (len + 1 > MAX_CARDS) throw logic_error("Cannot pack any more cards");

        int shift = BITS_PER_CARD * len;
        uint64_t slot_mask = MASK << shift;
        w = (w & ~slot_mask) | ((uint64_t(card_val + 1) & MASK) << shift);
        ++len;
    }

    int get(int idx) const {
        if (idx < 0 || idx >= len) throw logic_error("idx out of range");

        int shift = BITS_PER_CARD * idx;
        int v = int((w >> shift) & MASK);

        if (v == 0) throw logic_error("packed card was empty (it was zero)");
        return v - 1;
    }

    void del() {
        if (len <= 0) throw logic_error("del from empty PackedCards");
        int shift = BITS_PER_CARD * (len - 1);
        w &= ~(MASK << shift);
        --len;
    }

    bool operator==(const PackedCards& other) const noexcept {
        return w == other.w && len == other.len;
    }
};


struct InfoKey {
    PackedCards packed_cards;
    PackedActions packed_actions;

bool operator==(const InfoKey& o) const noexcept {
    return packed_cards.w == o.packed_cards.w &&
           packed_cards.len == o.packed_cards.len &&
           packed_actions.w == o.packed_actions.w &&
           packed_actions.len == o.packed_actions.len;
}
};

//Bobby black magic

struct InfoKeyHash {
    // kinda inspired by hash func used by benq (this one guy whos really smart)
    const uint64_t C = uint64_t(2e18*(3.14))+71;
    size_t operator()(const InfoKey& k) const noexcept {
        uint64_t combined = (k.packed_actions.w + 0x9e3779b97f4a7c15ULL) ^ (k.packed_cards.w * 0x517cc1b727220a95ULL);
        return __builtin_bswap64(combined * C);
    }
};

