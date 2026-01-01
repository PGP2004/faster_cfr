#include "InfoSet.h"
#include "GameState.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

using namespace std;


void InfoSet::make_actions(const GameState& state) {
    abs_and_concrete.clear();

    int p = state.get_pot();
    int my_pip = state.get_pip(state.get_active_player());

    // inc sizes (raise-by)
    int half = p / 2;
    int full = p;
    int two  = 2 * p;

    vector<pair<string, Action>> candidates = {
        {"fold",     {0, 0}},
        {"check",    {1, 0}},
        {"call",     {2, 0}},
        {"half_pot", {3, my_pip + half}},
        {"full_pot", {3, my_pip + full}},
        {"2x_pot",   {3, my_pip + two}},
    };

    for (auto &na : candidates) {
        if (state.is_legal_action(na.second)) abs_and_concrete.push_back(na);
    }

    size_t n = abs_and_concrete.size();
    strategy_sum.assign(n, 0.0);
    regret_sum.assign(n, 0.0);
}

    

InfoSet::InfoSet(const GameState& state):last_t(1) {
    make_actions(state);
}

void InfoSet::update_last_t(int t){
    last_t = t;
}

vector<double> InfoSet::get_regret_strategy() const {
    double total_sum = 0.0;
 
    vector<double> output(regret_sum.size(), 1.0 / regret_sum.size());

    for (double val : regret_sum) {
        total_sum += max(val, 0.0);
    }

    if (total_sum > 0.0){
        for (size_t i = 0;  i < regret_sum.size(); i++) {
            output[i] = max(0.0, regret_sum[i])/ total_sum;
        }
    }
    
    return output;
}
//todo:  get_actions_w_probs;;;

vector<pair<Action, double>> InfoSet::get_action_w_probs() const {
    vector<double> regret_probs = get_regret_strategy();

    vector<pair<Action, double>> output;
    output.reserve(regret_probs.size());

    for(size_t i = 0; i < regret_probs.size(); i++){
        output.push_back({abs_and_concrete[i].second, regret_probs[i]});
    }
    return output;

}

void InfoSet::update_regret(const vector<double>& action_deltas, int t) {

    double discount_factor = pow(double(last_t) / double(t), regret_discount_exp);

    if (regret_sum.size() != action_deltas.size()) {
        throw invalid_argument("Action deltas must meet regret_sum.size()");
    }
    
    for (size_t i = 0; i < regret_sum.size(); i++){
        regret_sum[i] = max(0.0, discount_factor*regret_sum[i]+action_deltas[i]); //cfr+ update
    }
}

void InfoSet::update_average_strategy(double reach_prob, int t) {

    if (reach_prob < 0.0 || reach_prob > 1.0) {
        throw invalid_argument("Reach probability must be between 0 and 1");
    }

    double discount_factor = pow(double(last_t) / double(t), strat_discount_exp);
    
    vector<double> cur_strat = get_regret_strategy();

    for (size_t i = 0; i < cur_strat.size(); i++){ 
        strategy_sum[i] = strategy_sum[i]*discount_factor + reach_prob * cur_strat[i];
    }
}

pair<Action, double> InfoSet::sample_regret_action(mt19937& rng) const {
    vector<double> probabilities = get_regret_strategy();
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


