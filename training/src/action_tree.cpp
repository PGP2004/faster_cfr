#include <cstdint>
#include <vector>
#include "action_tree.h"
#include "game_state.h"

using namespace std;

vector<Action> ActionTree::get_legal_actions(GameState& state) {
    int pot = state.get_pot();
    int my_pip = state.get_pip(state.get_active_player());

    vector<pair<string, Action>> candidates = {
        {"fold",  {0, 0}},
        {"check", {1, 0}},
        {"call",  {2, 0}},
        {"pot",   {3, my_pip + pot}},
    };

    vector<Action> action_vec;
    for (const auto& cand : candidates) {
        if (state.is_legal_action(cand.second)) {
            action_vec.push_back(cand.second);
        }
    }
    return action_vec;
}

void ActionTree::dfs_tree(GameState& state, mt19937& rng) {
    if (state.is_terminal_node()){
        return;
    }

    if (state.is_chance_node()) {
        ChanceUndo undo;
        state.write_chance_undo(undo);
        state.apply_chance(rng);
        dfs_tree(state, rng);
        state.undo_chance(undo);
        return;
    }

    vector<Action> legal_actions = get_legal_actions(state);

    for (size_t i = 0; i < legal_actions.size(); ++i) {
        Action action = legal_actions[i];

        size_t street_idx = state.get_street();   // street the action is taken in

        ActionUndo undo;
        state.write_action_undo(action, undo);
        state.apply_action(action);

        size_t child_idx = nodes.size();
        size_t saved_idx = cur_idx;

        nodes[cur_idx].child_idxs.push_back(child_idx);
        nodes[cur_idx].edges.push_back(action);

        int active_player = state.get_active_player();
        ActionNode child_node{child_idx, cur_idx, street_idx, {}, {}, active_player};
        nodes.push_back(child_node);

        cur_idx = child_idx;
        dfs_tree(state, rng);

        cur_idx = saved_idx;
        state.undo_action(undo);
    }
}

ActionTree::ActionTree(GameState& state) {
    mt19937 rng(0);
    ActionNode root{0, 0, state.get_street(), {}, {}, state.get_active_player()};
    nodes.push_back(root);
    root_idx = 0;
    cur_idx  = 0;
    dfs_tree(state, rng);
}