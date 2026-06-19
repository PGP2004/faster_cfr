
#pragma once
#include <vector>
#include <array>
#include <random>
#include <utility>   
#include <string>
#include <iostream>
#include <map>
#include <string>

using std::swap;
using std::string;
using std::array;
using std::mt19937;
using std::vector;

static uint8_t card_rank(uint8_t c) noexcept { return (uint8_t)(c / 4); }
static uint8_t card_suit(uint8_t c) noexcept { return (uint8_t)(c % 4); }

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
    array<int, 2> old_pips{0, 0};
    array<int, 2> old_stacks{0, 0};
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

uint32_t evaluate_raw(uint8_t* ranks, uint8_t* suits, uint8_t n);


