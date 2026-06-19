#pragma once

#include "Utils.h"
#include "Packings.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using std::array;
using std::string;
using std::vector;
using std::pair;
using std::uint64_t;
using std::unique_ptr;
using std::mt19937;
using std::logic_error;

class GameState {

private:

    static constexpr int starting_stack = 200;

    array<int, 5> board;
    array< array<int, 2>, 2> hands;

    array<int, 2> stacks;
    array<int, 2> pips;
    double p1_win_share;

    int pot;
    int street;
    int active_player;
    Action last_action;

    PackedActions packed_actions;
    array<PackedCards, 2> packed_cards;

    int abs_id_from_action(const Action& a) const;

public:

    GameState();
    GameState(const GameState&) = default;
    GameState& operator=(const GameState&) = default;

    double get_p1_winshare() const;
    double get_reward(int player) const;
    bool is_legal_action(const Action& action) const;

    void write_action_undo(const Action& action, ActionUndo& undo) const;
    void apply_action(const Action& action);
    void undo_action(const ActionUndo& undo_info);

    void write_chance_undo(ChanceUndo& undo) const;
    void apply_chance(mt19937& rng);
    void undo_chance(const ChanceUndo& undo);

    //below here is boilerplate

    inline bool is_terminal_node() const { return street == 8; }
    inline bool is_chance_node() const { return (street%2 == 0) && street != 8; }

    inline int get_active_player() const { return active_player; }
    inline int get_pot() const { return pot; }

    inline int get_pip(int player) const {
        if (player != 0 && player != 1) throw logic_error("The player index must be one of 1 or 0");
        return pips[player];
    }

    inline pair<int,int> get_hand(int player) const {
        if (player != 0 && player != 1) throw logic_error("The player index must be one of 1 or 0");
        return {hands[player][0], hands[player][1]};
    }

    inline InfoKey get_ID(int player) const {
        if (player != 0 && player != 1) throw logic_error("The player index must be one of 1 or 0");
        return InfoKey{packed_cards[player], packed_actions};
    }

    inline bool operator==(const GameState& o) const noexcept {
        return board == o.board
            && hands == o.hands
            && stacks == o.stacks
            && pips == o.pips
            && p1_win_share == o.p1_win_share
            && pot == o.pot
            && street == o.street
            && active_player == o.active_player
            && last_action == o.last_action
            && packed_actions == o.packed_actions
            && packed_cards == o.packed_cards;
    }


};
