#pragma once

#include "Utils.h"
#include "Packings.h"
#include "InfoKey.h"

#include <array>
#include <memory>
#include <random>
#include <utility>
#include <vector>

using std::array;
using std::string;
using std::vector;
using std::pair;
using std::uint64_t;
using std::unique_ptr;

// Minimal undo information for action application
struct ActionUndo {
    Action old_last_action;
    array<int, 2> old_pips;  // Need for round-end undo
    int to_pay;
    int old_street;  // Need for fold undo
    bool round_ended;
    int abs_id;  // Pre-computed abstract action ID
    int acting_player;  // Which player made the action (to refund correctly)
};

// Undo information for chance node (includes cards dealt)
struct ChanceUndo {
    array<int, 9> old_cards;  // 5 board + 4 hands
    array<int, 2> old_stacks;
    array<int, 2> old_pips;
    int old_pot;
};

class GameState {
private:
    static constexpr int starting_stack = 400;

    // References to externally-owned arrays
    array<int, 5>& board_ref;
    array<int, 4>& hands_ref; // p0,p0,p1,p1

    // Mutable game state
    array<int, 2> stacks; // p0,p1
    array<int, 2> pips;   // p0,p1
    int pot;
    int street;
    int active_player;
    Action last_action;

    // Packed abstractions / observations
    PackedActions packed_actions;
    array<PackedCards, 2> packed_cards;

    static void start_game(mt19937& rng,
                           array<int, 5>& board,
                           array<int, 4>& hands);

public:
    GameState(array<int, 5>& cur_board_ref,
              array<int, 4>& cur_hands_ref);

    GameState(array<int, 5>& cur_board_ref,
              array<int, 4>& cur_hands_ref,
              array<int, 2> cur_stacks,
              array<int, 2> cur_pips,
              int cur_pot,
              int cur_street,
              int cur_active_player,
              Action cur_last_action,
              PackedActions cur_packed_actions,
              array<PackedCards, 2> cur_packed_cards);

    bool is_legal_action(const Action& action) const;
    
    // Old API - creates new states (kept for tests)
    unique_ptr<GameState> sample_chance_node(std::mt19937& rng) const;
    unique_ptr<GameState> next_game_state(const Action& action) const;
    
    // New API - mutates state with undo/redo
    ActionUndo apply_action(const Action& action);
    void undo_action(const ActionUndo& undo_info, const Action& action);
    ChanceUndo apply_chance(std::mt19937& rng);
    void undo_chance(const ChanceUndo& undo_info);
    
    double get_rewards(int player) const;
    InfoKey get_ID(int player) const;

    // Inline simple getters for performance
    inline bool is_terminal() const {
        return street == 8;
    }

    inline bool is_chance_node() const {
        return (street % 2 == 0 && street != 8);
    }

    inline int get_active_player() const {
        return active_player;
    }

    inline int get_pot() const {
        return pot;
    }

    inline int get_pip(int player) const {
        if (player != 0 && player != 1) {
            throw std::logic_error("The player index must be one of 1 or 0");
        }
        return pips[player];
    }

    inline array<int, 2> get_hand(int player) const {
        if (player != 0 && player != 1) {
            throw std::logic_error("The player index must be one of 1 or 0");
        }
        return {hands_ref[2 * player], hands_ref[2 * player + 1]};
    }

    inline array<int, 5> get_board() const {
        int n = 0;
        if (street < 2) n = 0;
        else if (street < 4) n = 3;
        else if (street < 6) n = 4;
        else n = 5;

        array<int, 5> output;
        output.fill(-1);
        for (int i = 0; i < n; i++) output[i] = board_ref[i];
        return output;
    }

    void add_legal_actions(int my_pip, int p, vector<pair<string, Action>>& abs_and_concrete) const;
};
