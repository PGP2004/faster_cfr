
#pragma once

#include <random>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <unordered_map>

#include "game_state.h"
#include "info_sets.h"

#include "ankerl/unordered_dense.h"

using std::mt19937;
using std::string;
using std::vector;
using std::array;
using std::unique_ptr;
using std::shared_ptr;


//TODO: Add  InfoKey to CFR

class CFR {

    private:
        GameState state;
        Abstraction abs;
        ActionTree action_tree;
        mt19937 rng;

        InfoSets infosets;
        InfoKey get_InfoKey(const GameState& state, const ActionTree& at);
        double traverse(int player, GameState& state, ActionTree& at, int t);

    public:

        CFR(GameState init_game_state, Abstraction& abstraction, ActionTree& action_tree);
        void train(int num_iterations, int starting_iter);
        double get_action_prob(int player, InfoKey info_key, Action action);

    };