#include "GameState.h"
#include "InfoSet.h"
#include "CFR.h"
#include "Profiler.h"
#include "MinimalProfiler.h"
#include "VectorPool.h"
#include <memory>
#include <utility>
#include <iostream>

//I templated this to save time on infoset stuff could undo in theory

//might wanna template over abstractiosn?


using namespace std;

CFR::CFR(uint32_t seed, unique_ptr<GameState> init_game_state)
    : init_state(std::move(init_game_state)), rng(seed) {
    // Pre-reserve capacity to avoid rehashing during training
    // We expect ~4.5M InfoSets per player based on profiling
    infoset_dict[0].reserve(5000000);
    infoset_dict[1].reserve(5000000);
    
    // Pre-allocate vector pool to avoid allocations during traverse
    // Max recursion depth is ~50-100 in poker, allocate 200 to be safe
    VectorPool::preallocate(200);
}

InfoSet& CFR::get_InfoSet(int player, const GameState& state) {
    PROFILE_FUNCTION();
    InfoKey id = state.get_ID(player);

    auto& mp = infoset_dict[player];
    
    // Check if already exists
    {
        TIME_HASH_LOOKUP();
        auto it = mp.find(id);
        if (it != mp.end()) {
            return *(it->second);  // Dereference the shared_ptr
        }
    }
    
    // Not found - construct InfoSet in a shared_ptr and insert
    auto new_info = std::make_shared<InfoSet>(state);
    {
        TIME_HASH_INSERT();
        auto insert_result = mp.insert(std::make_pair(id, new_info));
        return *(insert_result.first->second);  // Dereference the shared_ptr
    }
}

double CFR::traverse(int player, GameState& state, double pi_i, double pi_opp, int t) {
    PROFILE_FUNCTION();
    if (state.is_terminal()) {
        return state.get_rewards(player);
    }

    if (state.is_chance_node()) {
        ChanceUndo undo = state.apply_chance(rng);
        double util = traverse(player, state, pi_i, pi_opp, t);
        state.undo_chance(undo);
        return util;
    }

    int current_player = state.get_active_player();

    if (current_player != player) {
        InfoSet& infoset = get_InfoSet(current_player, state);
        auto [sampled_action, sample_prob] = infoset.sample_regret_action(rng);
        ActionUndo undo = state.apply_action(sampled_action);
        double util = traverse(player, state, pi_i, pi_opp * sample_prob, t);
        state.undo_action(undo, sampled_action);
        return util;
    }

    if (current_player != player){
        throw logic_error("This should not be possible");
    }
    
    InfoSet& infoset = get_InfoSet(player, state);

    // Get buffers from pool (RAII - automatically returned on scope exit)
    VectorPool::ActionBuffer action_buf;
    VectorPool::ProbsBuffer probs_buf;
    VectorPool::DeltaBuffer delta_buf;
    
    auto& actions_with_probs = action_buf.get();
    auto& probs = probs_buf.get();
    auto& action_deltas = delta_buf.get();
    
    // Zero-allocation: fills buffers directly
    infoset.get_action_w_probs_fast(actions_with_probs, probs);
    action_deltas.reserve(actions_with_probs.size());
    
    double node_util = 0.0;

    for (size_t i = 0; i < actions_with_probs.size(); i++) {
        const auto& act_and_prob = actions_with_probs[i];
        ActionUndo undo = state.apply_action(act_and_prob.first);
        double action_util = traverse(player, state, pi_i * act_and_prob.second, pi_opp, t);
        state.undo_action(undo, act_and_prob.first);
        
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
    PROFILE_FUNCTION();
    for (int i = 0; i < num_iterations; ++i) {
        traverse(0, *init_state, 1.0, 1.0, i);
        traverse(1, *init_state, 1.0, 1.0, i);
    }
}
