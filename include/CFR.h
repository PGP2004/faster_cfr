
    #pragma once

    #include <random>
    #include <string>
    #include <unordered_map>
    #include <vector>
    #include <array>
    #include <memory>

    #include "GameState.h"
    #include "InfoSet.h"

    using std::mt19937;
    using std::string;
    using std::unordered_map;
    using std::vector;
    using std::array;
    using std::unique_ptr;


class CFR {

    private:

        unique_ptr<GameState> init_state;

        array<unordered_map<InfoKey, InfoSet, InfoKeyHash>,2> infoset_dict;

        mt19937 rng;

        InfoSet& get_InfoSet(int player, const GameState& state);

        double traverse(int player,  const GameState& state, double pi_i, double pi_opp, int t);

    public:
    
        const array<unordered_map<string, InfoSet>, 2>& view_infosets() const;
        
        explicit CFR(uint32_t seed, unique_ptr<GameState> init_game_state);

        void train(int num_iterations);


    };