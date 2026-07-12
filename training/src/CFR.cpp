#include "game_state.h"
#include "info_set.h"
#include "cfr.h"

#include "vector_pool.h"
#include <memory>
#include <utility>
#include <iostream>

#include <iomanip>
#include <map>

using MapT =  ankerl::unordered_dense::map<InfoKey, shared_ptr<InfoSet>, InfoKeyHash>;
using namespace std;

CFR::CFR(uint32_t seed, int infoset_prealloc, int vectorpool_prealloc, GameState init_game_state, Abstraction game_abs)
    : init_state(std::move(init_game_state)), rng(seed), game_abs(game_abs){
    
    infoset_dict[0].reserve(infoset_prealloc);
    infoset_dict[1].reserve(infoset_prealloc);

    VectorPool::preallocate(vectorpool_prealloc);
}

InfoSet& CFR::get_InfoSet(int player, const GameState& state) {

    InfoKey id = state.get_ID(player);
    id.hand_id = game_abs.cluster_of(id.street, id.hand_id);   // now holds cluster id

    auto& mp = infoset_dict[player];

    auto it = mp.find(id);
     if (it != mp.end()) {
        return *(it->second);  
    }

    auto new_info = std::make_shared<InfoSet>(state);

    auto insert_result = mp.insert(std::make_pair(id, new_info));
    return *(insert_result.first->second);  
}

// double CFR::traverse(int player, GameState& state, double pi_i, int t) {

//     if (state.is_terminal_node()) {
//         return state.get_reward(player);
//     }

//     // cout << "Traverse Chkpt 1" << endl;

//     if (state.is_chance_node()) {

//         ChanceUndo undo;
//         state.write_chance_undo(undo);
//         state.apply_chance(rng);
//         double util = traverse(player, state, pi_i, t);
//         state.undo_chance(undo);
//         return util;
//     }

//     //  cout << "Traverse Chkpt 2" << endl;
//     // Player Action Branch

//     int active_player = state.get_active_player();

//     if (active_player != player) {
//         InfoSet& infoset = get_InfoSet(active_player, state);
//         VectorPool::ProbsBuffer probs_buf;
//         auto& probs = probs_buf.get();
//         auto [sampled_action, sample_prob] = infoset.sample_regret_action(rng, probs);
                
//         ActionUndo undo;
//         state.write_action_undo(sampled_action, undo);
//         state.apply_action(sampled_action);
//         double util = traverse(player, state, pi_i, t);
//         state.undo_action(undo);
//         return util;
//     }

//     // cout << "Traverse Chkpt 3" << endl;
//     // Branch where active player = current player

//     InfoSet& infoset = get_InfoSet(player, state);

//     VectorPool::ActionBuffer action_buf;
//     VectorPool::ProbsBuffer probs_buf;
//     VectorPool::DeltaBuffer delta_buf;
//     vector<Action>& actions = action_buf.get();
    
//     vector<double>& probs = probs_buf.get();
//     vector<double>& action_deltas = delta_buf.get();

//     infoset.get_action_w_probs(actions, probs);
//     action_deltas.reserve(actions.size());
//     double node_util = 0.0;

//     for (size_t i = 0; i < actions.size(); i++) {
//         ActionUndo undo;
//         state.write_action_undo(actions[i], undo);
//         state.apply_action(actions[i]);
//         double action_util = traverse(player, state, pi_i * probs[i], t);
//         state.undo_action(undo);
//         node_util += probs[i] * action_util;
//         action_deltas.push_back(action_util);
//     }

//     for (size_t i = 0; i < action_deltas.size(); ++i) {
//         action_deltas[i] = action_deltas[i] - node_util;
//     }

//     infoset.update_regret(action_deltas, t);
//     infoset.update_average_strategy(pi_i, probs, t);
//     infoset.update_last_t(t);

//     return node_util;
// }


double CFR::traverse(int player, GameState& state, int t) {
    if (state.is_terminal_node()) {
        return state.get_reward(player);
    }
    if (state.is_chance_node()) {
        ChanceUndo undo;
        state.write_chance_undo(undo);

        state.apply_chance(rng);
        double util = traverse(player, state, t);
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
       
        infoset.update_average_strategy(1.0, probs, t);
        infoset.update_last_t(t);

        ActionUndo undo;
        state.write_action_undo(sampled_action, undo);

        state.apply_action(sampled_action);
        double util = traverse(player, state, t);
        
        state.undo_action(undo);
        return util;
    }
    // Branch where active player = current player
    InfoSet& infoset = get_InfoSet(player, state);
    VectorPool::ActionBuffer action_buf;

    VectorPool::ProbsBuffer probs_buf;
    VectorPool::DeltaBuffer delta_buf;

    vector<Action>& actions = action_buf.get();
    vector<double>& probs = probs_buf.get();
    vector<double>& action_deltas = delta_buf.get();

    infoset.get_action_w_probs(actions, probs);
    action_deltas.reserve(actions.size());

    double node_util = 0.0;

    for (size_t i = 0; i < actions.size(); i++) {

        ActionUndo undo;
        state.write_action_undo(actions[i], undo);

        state.apply_action(actions[i]);
        double action_util = traverse(player, state, t);
        state.undo_action(undo);

        node_util += probs[i] * action_util;
        action_deltas.push_back(action_util);
    }
    for (size_t i = 0; i < action_deltas.size(); ++i) {

        action_deltas[i] = action_deltas[i] - node_util;
    }

    infoset.update_regret(action_deltas, t);
    infoset.update_last_t(t);
    return node_util;
}

double CFR::get_action_prob(int player, InfoKey key, string action_name){

    key.hand_id = game_abs.cluster_of(key.street, key.hand_id);   // now holds cluster id
    auto& mp = infoset_dict[player];
    auto it = mp.find(key);

    if (it == mp.end()){
        cout << "cannot find a prob. (cant find infoset)" << endl;
        return -1.0;
    }

    InfoSet& iset = *(it->second); 
    unordered_map<string, double> avg_strat = iset.get_average_strategy(); 

    if (avg_strat.find(action_name) == avg_strat.end()){
        cout << "could not find the probability for the action: " << action_name << endl;
        return -1.0;
    }
    
    return avg_strat[action_name];
    }


void CFR::train(int num_iterations, int starting_iter) {
    for (int i = starting_iter; i < starting_iter+ num_iterations; ++i) {
        traverse(0, init_state, i);
        // cout << "Did we finish the first traverse " << endl;
        traverse(1, init_state, i);
    }
}

