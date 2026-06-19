#include "InfoSet.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>

using namespace std;


void InfoSet::make_actions(const GameState& state) {

    //this is responsible for collapsing into discretized action space
    abs_and_concrete.clear();

    int pot = state.get_pot();
    int my_pip = state.get_pip(state.get_active_player());

    vector<pair<string, Action>> candidates = {
        {"fold",     {0, 0}},
        {"check",    {1, 0}},
        {"call",     {2, 0}},

        {"half_pot", {3, my_pip + pot/2}},
        {"pot",      {3, my_pip + pot}},
        {"2x_pot",   {3, my_pip + 2*pot}},
    };

    for (const auto& cand : candidates) {
        if (state.is_legal_action(cand.second)) {
            abs_and_concrete.push_back(cand);
        }
    }

    size_t n = abs_and_concrete.size();
    strategy_sum.assign(n, 0.0);
    regret_sum.assign(n, 0.0);
}


InfoSet::InfoSet(const GameState& state) {
    make_actions(state);
}

void InfoSet::update_regret(const vector<double>& action_deltas, int t) {
    if (regret_sum.size() != action_deltas.size()) {
        throw invalid_argument("Action_deltas and regret_sum must have the same size");
    }

    for (size_t i = 0; i < regret_sum.size(); i++) {
        regret_sum[i] += double(t) * action_deltas[i];  
    }
}


void InfoSet::get_regret_strategy(vector<double>& output) const {

    output.resize(regret_sum.size());
    double total_sum = 0.0;

    for (double val : regret_sum) {
        total_sum += max(val, 0.0);
    }
    
    if (total_sum > 0.0) {
        for (size_t i = 0; i < regret_sum.size(); i++) {
            output[i] = max(0.0, regret_sum[i]) / total_sum;
        }

    } else {
        double uniform = 1.0 / regret_sum.size();
        for (size_t i = 0; i < regret_sum.size(); i++) {
            output[i] = uniform;
        }
    }
}

void InfoSet::update_average_strategy(double reach_prob, vector<double>& cur_strat, int t) {
    if (reach_prob < 0.0 || reach_prob > 1.0) {
        throw invalid_argument("Reach probability must be between 0 and 1");
    }
    if (cur_strat.size() != strategy_sum.size()) throw logic_error("size mismatch");

    for (size_t i = 0; i < cur_strat.size(); i++) {
        strategy_sum[i] += double(t) * reach_prob * cur_strat[i];
    }
}

pair<Action, double> InfoSet::sample_regret_action(mt19937& rng, vector<double>& probabilities) const {
    
    get_regret_strategy(probabilities);
    
    discrete_distribution<int> dist(probabilities.begin(), probabilities.end());
    int sampled_idx = dist(rng);
    
    return {abs_and_concrete[sampled_idx].second, probabilities[sampled_idx]};
}

unordered_map<string, double> InfoSet::get_average_strategy() const{
    
    unordered_map<string, double> output;
    output.reserve(abs_and_concrete.size());

    double sum = 0.0;
    for (double x : strategy_sum){
        sum += x;
    }

    if (sum <= 0.0) {
        double p = 1.0/double(abs_and_concrete.size()); 
        for (size_t i = 0; i < abs_and_concrete.size(); ++i) {
            output[abs_and_concrete[i].first] = p;
        }
        return output;
    }

    for (size_t i = 0; i < abs_and_concrete.size(); ++i) {
        output[abs_and_concrete[i].first] = strategy_sum[i] / sum;
    }

    return output;
}

void InfoSet::update_last_t(int t){
    last_t = t;
}

void InfoSet::get_action_w_probs(vector<pair<Action, double>>& actions_out, vector<double>& probs_out) const {
    
    size_t n = regret_sum.size();
    
    probs_out.resize(n);
    actions_out.resize(n);

    get_regret_strategy(probs_out);
    actions_out.resize(n);

    for (size_t i = 0; i < n; i++) {
        actions_out[i] = {abs_and_concrete[i].second, probs_out[i]};
    }
}