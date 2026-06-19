#include "GameState.h"
#include "InfoSet.h"
#include "CFR.h"

#include "VectorPool.h"
#include <memory>
#include <utility>
#include <iostream>

#include <iomanip>
#include <map>

//TODO: Review


using namespace std;

CFR::CFR(uint32_t seed, int infoset_prealloc, int vectorpool_prealloc, unique_ptr<GameState> init_game_state)
    : init_state(std::move(init_game_state)), rng(seed) {

    infoset_dict[0].reserve(infoset_prealloc);
    infoset_dict[1].reserve(infoset_prealloc);

    VectorPool::preallocate(vectorpool_prealloc);
}

//TODO: reunderstand this function
InfoSet& CFR::get_InfoSet(int player, const GameState& state) {

    InfoKey id = state.get_ID(player);

    auto& mp = infoset_dict[player];

    auto it = mp.find(id);
     if (it != mp.end()) {
        return *(it->second);  
    }

    auto new_info = std::make_shared<InfoSet>(state);

    auto insert_result = mp.insert(std::make_pair(id, new_info));
    return *(insert_result.first->second);  

}

double CFR::traverse(int player, GameState& state, double pi_i, double pi_opp, int t) {

    if (state.is_terminal_node()) {
        return state.get_reward(player);
    }

    if (state.is_chance_node()) {

        ChanceUndo undo;
        state.write_chance_undo(undo);
        state.apply_chance(rng);
        double util = traverse(player, state, pi_i, pi_opp, t);
        state.undo_chance(undo);
        return util;
    }

    // Player Action Branch

    int active_player = state.get_active_player();

    if (active_player != player) {
        InfoSet& infoset = get_InfoSet(active_player, state);
        VectorPool::ProbsBuffer probs_buf;
        auto& probs = probs_buf.get();
        auto [sampled_action, sample_prob] = infoset.sample_regret_action(rng, probs);
                
        ActionUndo undo;
        state.write_action_undo(sampled_action, undo);
        state.apply_action(sampled_action);
        double util = traverse(player, state, pi_i, pi_opp * sample_prob, t);
        state.undo_action(undo);
        return util;
    }

    // Branch where active player = current player

    InfoSet& infoset = get_InfoSet(player, state);

    VectorPool::ActionBuffer action_buf;
    VectorPool::ProbsBuffer probs_buf;
    VectorPool::DeltaBuffer delta_buf;
    vector<pair<Action, double>>& actions_with_probs = action_buf.get();
    
    vector<double>& probs = probs_buf.get();
    vector<double>& action_deltas = delta_buf.get();

    infoset.get_action_w_probs(actions_with_probs, probs);
    action_deltas.reserve(actions_with_probs.size());
    double node_util = 0.0;

    for (size_t i = 0; i < actions_with_probs.size(); i++) {
        const pair<Action, double>& act_and_prob = actions_with_probs[i];

        ActionUndo undo;
        state.write_action_undo(act_and_prob.first, undo);
        state.apply_action(act_and_prob.first);
        double action_util = traverse(player, state, pi_i * act_and_prob.second, pi_opp, t);
        state.undo_action(undo);
        
        node_util += act_and_prob.second * action_util;
        action_deltas.push_back(action_util);
    }

    for (size_t i = 0; i < action_deltas.size(); ++i) {
        action_deltas[i] = action_deltas[i] - node_util;
    }

    infoset.update_regret(action_deltas, t);
    infoset.update_average_strategy(pi_i, probs, t);
    infoset.update_last_t(t);

    return node_util;
}

void CFR::train(int num_iterations) {
    for (int i = 0; i < num_iterations; ++i) {
        traverse(0, *init_state, 1.0, 1.0, i);
        traverse(1, *init_state, 1.0, 1.0, i);
    }
}

