#pragma once

#include "Utils.h"
#include "Packings.h"

#include <array>
#include <memory>
#include <random>
#include <utility>
#include <vector>

using std::array;
using std::vector;
using std::pair;
using std::uint64_t;
using std::unique_ptr;

class GameState {
private:
    static constexpr int starting_stack = 400;

    array<int, 5>& board_ref;
    array<int, 4>& hands_ref; // p0,p0,p1,p1

    array<int, 2> stacks; 
    array<int, 2> pips;   
    int pot;
    int street;
    int active_player;

    // vector<Action> action_history;
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
            //   vector<Action> cur_history,
              Action cur_last_action,
              PackedActions cur_packed_actions,
              array<PackedCards, 2> cur_packed_cards);

    bool is_legal_action(const Action& action) const;
    bool is_terminal() const;
    bool is_chance_node() const;

    unique_ptr<GameState> sample_chance_node(std::mt19937& rng) const;
    unique_ptr<GameState> next_game_state(const Action& action) const;

    double get_rewards(int player) const;

    int get_active_player() const;
    int get_pot() const;
    int get_pip(int player) const;

    // const std::vector<Action>& get_action_history() const;

    array<int, 2> get_hand(int player) const;
    array<int, 5> get_board() const;

    // Note: your .cpp currently defines this *non-const*. Match that.
    pair<uint64_t, uint64_t> get_ID(int player) const;
};
