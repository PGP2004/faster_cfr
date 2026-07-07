
#pragma once

#include <random>
#include <string>
#include <vector>
#include <array>
#include <memory>

#include "game_state.h"
#include "info_set.h"
#include "packings.h"
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

        unique_ptr<GameState> init_state;
        mt19937 rng;

        Abstraction game_abs;
        array<ankerl::unordered_dense::map<InfoKey, shared_ptr<InfoSet>, Abstraction>,2> infoset_dict;

        InfoSet& get_InfoSet(int player, const GameState& state);

        double traverse(int player, GameState& state, double pi_i, int t);

    public:
        
        CFR(uint32_t seed, int infoset_prealloc, int vectorpool_prealloc, unique_ptr<GameState> init_game_state, Abstraction game_abs);

        void train(int num_iterations);

    };