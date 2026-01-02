#pragma once
#include <cstdint>
#include <utility>

// InfoSet key: (packed_cards, packed_actions, packed_state)
// packed_state contains pot, my_pip, opp_pip (each ≤ 1024, using 10 bits each)
struct InfoKey {
    uint64_t cards;
    uint64_t actions;
    uint64_t packed_state;  // pot | (my_pip << 10) | (opp_pip << 20)
    
    // Helper to pack pot/pip values
    static inline uint64_t pack_state(int pot, int my_pip, int opp_pip) {
        return static_cast<uint64_t>(pot) | 
               (static_cast<uint64_t>(my_pip) << 10) | 
               (static_cast<uint64_t>(opp_pip) << 20);
    }
    
    bool operator==(const InfoKey& other) const {
        return cards == other.cards && actions == other.actions && packed_state == other.packed_state;
    }
};

// struct InfoKeyHash {
//     // kinda inspired by hash func used by benq (this one guy whos really smart)
//     const uint64_t C = uint64_t(2e18*(3.14))+71;
//     size_t operator()(const InfoKey& k) const noexcept {
//         uint64_t combined = k.cards ^ (k.actions + 0x9e3779b97f4a7c15ULL) ^ (k.packed_state * 0x517cc1b727220a95ULL);
//         return __builtin_bswap64(combined * C);
//     }
// };

struct InfoKeyHash {
    size_t operator()(const InfoKey& k) const noexcept {
        return static_cast<size_t>(k.cards ^ k.actions ^ k.packed_state);
    }
};