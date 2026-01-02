#include "GameState.h"
#include "InfoSet.h"
#include "CFR.h"
#include <memory>
#include <utility>
#include <iostream>

//I templated this to save time on infoset stuff could undo in theory

//might wanna template over abstractiosn?
using namespace std;

CFR::CFR(uint32_t seed, unique_ptr<GameState> init_game_state)
    : init_state(std::move(init_game_state)), rng(seed) {}

InfoSet& CFR::get_InfoSet(int player, const GameState& state) {
    InfoKey id = state.get_ID(player);

    auto& mp = infoset_dict[player];                
    auto [it, inserted] = mp.try_emplace(id, state); 
    return it->second;
}

double CFR::traverse(int player, const GameState& state, double pi_i, double pi_opp, int t) {
    if (state.is_terminal()) {
        return state.get_rewards(player);
    }

    if (state.is_chance_node()) {
        unique_ptr<GameState> next_state = state.sample_chance_node(rng);
        return traverse(player, *next_state, pi_i, pi_opp, t);
    }

    int current_player = state.get_active_player();

    if (current_player != player) {
        InfoSet& infoset = get_InfoSet(current_player, state);
        auto [sampled_action, sample_prob] = infoset.sample_regret_action(rng);
        unique_ptr<GameState> next_state = state.next_game_state(sampled_action);
        return traverse(player, *next_state, pi_i, pi_opp * sample_prob, t);
    }

    if (current_player != player){
        throw logic_error("This should not be possible");
    }
    InfoSet& infoset = get_InfoSet(player, state);

    vector<pair<Action, double>> actions_with_probs = infoset.get_action_w_probs();
    double node_util = 0.0;

    vector<double> action_deltas;
    action_deltas.reserve(actions_with_probs.size());

    for (const pair<Action,double>& act_and_prob : actions_with_probs) {
        unique_ptr<GameState> next_state = state.next_game_state(act_and_prob.first);
        double action_util = traverse(player, *next_state, pi_i * act_and_prob.second, pi_opp, t);
        node_util += act_and_prob.second * action_util;
        action_deltas.push_back(action_util);
    }

    for (size_t i = 0; i < action_deltas.size(); ++i) {
        action_deltas[i] = pi_opp * (action_deltas[i] - node_util);
    }

    infoset.update_regret(action_deltas, t); 
    infoset.update_average_strategy(pi_i, t);

    return node_util;
}

void CFR::train(int num_iterations) {
    for (int i = 0; i < num_iterations; ++i) {
        traverse(0, *init_state, 1.0, 1.0, i);
        traverse(1, *init_state, 1.0, 1.0, i);
    }
}
