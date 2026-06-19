
#pragma once

#include <random>
#include <string>
#include <vector>
#include <array>
#include <memory>

#include "GameState.h"
#include "InfoSet.h"
#include "Packings.h"
#include "ankerl/unordered_dense.h"

using std::mt19937;
using std::string;
using std::vector;
using std::array;
using std::unique_ptr;
using std::shared_ptr;


class CFR {

    private:

        unique_ptr<GameState> init_state;

        array<ankerl::unordered_dense::map<InfoKey, shared_ptr<InfoSet>, InfoKeyHash>,2> infoset_dict;

        mt19937 rng;

        InfoSet& get_InfoSet(int player, const GameState& state);

        double traverse(int player, GameState& state, double pi_i, double pi_opp, int t);

    public:
        
        explicit CFR(uint32_t seed, int infoset_prealloc, int vectorpool_prealloc, unique_ptr<GameState> init_game_state);

        void train(int num_iterations);

    };