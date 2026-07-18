
#pragma once
#include <vector>
#include <array>
#include <random>
#include <utility>   
#include <string>
#include <iostream>
#include <map>
#include <string>

#include <iostream>
#include <fstream>

extern "C" {
#define _Bool bool
#include "hand_index.h"
#undef _Bool
}

struct Indexer {
    hand_indexer_t h;
    Indexer(uint32_t rounds, const uint8_t* cpr) { hand_indexer_init(rounds, cpr, &h); }
    ~Indexer() { hand_indexer_free(&h); }
    Indexer(const Indexer&) = delete; 
    Indexer& operator=(const Indexer&) = delete;
};

struct Action{
    int type;
    int amt;
    bool operator==(const Action&) const = default;

    void print() const {
        // type: fold=0, check=1, call=2, raise=3
        const char* name;
        switch (type) {
            case -1: name = "placeholder"; break;
            case  0: name = "fold";        break;
            case  1: name = "check";       break;
            case  2: name = "call";        break;
            case  3: name = "raise";       break;
            default: name = "UNKNOWN";     break;
        }
        std::cout << "Action(type=" << name << ", amt=" << amt << ")";
    }

};

struct ActionUndo {
    Action old_last_action{0, 0};
    int old_street = 0;
    int old_active_player = 0;
    int old_to_pay = 0;

    bool operator==(const ActionUndo&) const = default;
};

struct ChanceUndo {
    std::array<int, 2> old_pips{0, 0};
    std::array<int, 2> old_stacks{0, 0};
    int old_pot = 0;
    int old_active_player = 0;
    int old_street = 0;
    Action old_last_action{-1, -1};
    
    bool operator==(const ChanceUndo&) const = default;
    
};

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);
inline uint8_t card_rank(uint8_t c) noexcept { return (uint8_t)(c / 4); }
inline uint8_t card_suit(uint8_t c) noexcept { return (uint8_t)(c % 4); }
