
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


//TODO: Ensure they match the one used by the indexer


static uint8_t parse_rank(char rc) {
    rc = (char)std::toupper((unsigned char)rc);
    switch (rc) {
        case '2': return 0;
        case '3': return 1;
        case '4': return 2;
        case '5': return 3;
        case '6': return 4;
        case '7': return 5;
        case '8': return 6;
        case '9': return 7;
        case 'T': return 8;
        case 'J': return 9;
        case 'Q': return 10;
        case 'K': return 11;
        case 'A': return 12;
        default: throw std::invalid_argument("Bad rank character");
    }
}

static uint8_t parse_suit(char sc) {
    switch (sc) {
        case 'c': return 0;
        case 'd': return 1;
        case 'h': return 2;
        case 's': return 3;
        default: throw std::invalid_argument("Bad suit character");
    }
}

static uint8_t string_to_card(char rc, char sc) {
    uint8_t rank_int = parse_rank(rc);  
    uint8_t suit_int = parse_suit(sc); 
    return static_cast<uint8_t>(deck_make_card(suit_int, rank_int));
}

inline uint8_t card_rank(uint8_t c) noexcept { return (uint8_t)(c / 4); }
inline uint8_t card_suit(uint8_t c) noexcept { return (uint8_t)(c % 4); }

struct Action{
    int type;
    int amt;

     bool operator==(const Action& other) const noexcept {
        return type == other.type && amt == other.amt;
    }

    void print() const {
        // type: fold=0, check=1, call=2, raise=3
        static const std::map<int, std::string> type_name = {
            {-1, "placeholder"},
            {0, "fold"},
            {1, "check"},
            {2, "call"},
            {3, "raise"},
        };

        auto it = type_name.find(type);
        const std::string name = (it == type_name.end() ? "UNKNOWN" : it->second);

        std::cout << "Action(type=" << name << ", amt=" << amt << ")"; // flush
    }

};

struct ActionUndo {
    Action old_last_action{0, 0};
    int old_street = 0;
    int old_active_player = 0;
    int old_to_pay = 0;

    void print() const {
        std::cout << "ActionUndo\n";
        std::cout << "  old_last_action: ";
        old_last_action.print();
        std::cout << std::endl;
        std::cout << "  old_street: " << old_street << "\n";
        std::cout << "  old_active_player: " << old_active_player << "\n";
        std::cout << "  old_to_pay: " << old_to_pay << "\n";
    }

    bool operator==(const ActionUndo& other) const noexcept {
        return old_last_action == other.old_last_action
            && old_street == other.old_street
            && old_active_player == other.old_active_player
            && old_to_pay == other.old_to_pay;
    }
};

struct ChanceUndo {
    std::array<int, 2> old_pips{0, 0};
    std::array<int, 2> old_stacks{0, 0};
    int old_pot = 0;
    int old_active_player = 0;
    int old_street = 0;
    Action old_last_action{-1, -1};

   void print() const {
        std::cout << "ChanceUndo\n";
        std::cout << "  old_street: " << old_street << "\n";
        std::cout << "  old_active_player: " << old_active_player << "\n";
        std::cout << "  old_pot: " << old_pot << "\n";
        std::cout << "  old_stacks: [" << old_stacks[0] << ", " << old_stacks[1] << "]\n";
        std::cout << "  old_pips:   [" << old_pips[0] << ", " << old_pips[1] << "]\n";
        std::cout << "  old_last_action: ";
        old_last_action.print();
        std::cout << std::endl;
    }

    bool operator==(const ChanceUndo& other) const noexcept {
        return old_pips == other.old_pips
            && old_stacks == other.old_stacks
            && old_pot == other.old_pot
            && old_active_player == other.old_active_player
            && old_street == other.old_street
            && old_last_action == other.old_last_action;
    }
};


struct DataHeader{
    uint64_t round; //0 =preflop , 1 = flop, 2 = turn, 3 = river
    uint64_t num_rows;
    uint64_t num_cols;
    uint64_t bytes_per_elt;
    bool operator==(const DataHeader&) const = default;

    std::string to_string() const {
        return "DataHeader{round=" + std::to_string(round) + 
        + ", num_rows=" + std::to_string(num_rows)
        + ", num_cols=" + std::to_string(num_cols)
        + ", bytes_per_elt=" + std::to_string(bytes_per_elt) + "}";
    }
};

//TODO: Clean up the data loading across my scripts
template <typename T>
std::pair<std::vector<T>, DataHeader> load_matrix_and_header(const std::string& result_path) {
    std::ifstream in(result_path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + result_path);

    DataHeader header;
    in.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (in.gcount() != static_cast<std::streamsize>(sizeof(header))) throw std::runtime_error("missing header");
    
    if (header.bytes_per_elt != sizeof(T)) throw std::runtime_error("Wrong size type compared to header");

    uint64_t body_bytes = header.num_rows * header.num_cols * header.bytes_per_elt;

    std::streampos here = in.tellg(); in.seekg(0, std::ios::end);
    std::streampos end = in.tellg(); in.seekg(here);
    if (static_cast<uint64_t>(end - here) != body_bytes) throw std::runtime_error("body size does not match header");

    std::vector<T> results(body_bytes / sizeof(T));
    in.read(reinterpret_cast<char*>(results.data()), static_cast<std::streamsize>(body_bytes));
    if (static_cast<uint64_t>(in.gcount()) != body_bytes) throw std::runtime_error("truncated body");

    return {std::move(results), header};
}

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);


