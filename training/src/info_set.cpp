#include "info_sets.h"
#include "action_tree.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <cmath>

using namespace std;

InfoSets::InfoSets(const ActionTree& action_tree, const Abstraction& abstraction) {

    vector<size_t> cluster_counts(4,0);
    cluster_counts[0] = abstraction.preflop_clusters.size();
    cluster_counts[1] = abstraction.flop_clusters.size();
    cluster_counts[2] = abstraction.turn_clusters.size();
    cluster_counts[3] = abstraction.river_clusters.size();

    int total = 0;

    for (ActionNode node : action_tree.nodes){

        size_t st = node.street_idx;
        size_t num_actions = node.edges.size();
        size_t num_clusters = cluster_counts[st];

        for (size_t c = 0; c < num_clusters; ++ c){
            size_t offset = offsets[offsets.size()-1] + num_actions;
            offsets.push_back(offset);
        }

        total += num_actions*num_clusters;
    }

    regret_sum.assign(total, 0.0);
    strategy_sum.assign(total, 0.0);
}


void InfoSets::update_regret(const InfoKey& ikey, const vector<double>& action_deltas) {

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();

    if (n != action_deltas.size()) {
        throw invalid_argument("Action_deltas and regret_sum must have the same size");
    }

    for (size_t i = 0; i < n; i++) {
        regret_sum[offset+i] += action_deltas[i];  
    }
}

void InfoSets::update_strategy(const InfoKey& ikey , vector<double>& cur_strat) {

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();

    if (n != strategy_sum.size()) throw logic_error("size mismatch");

    for (size_t i = 0; i < n; i++) {
        strategy_sum[offset+i] += cur_strat[i];
    }
}


void InfoSets::get_regret_strategy(const InfoKey& ikey, vector<double>& output) const{

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();
    
    output.resize(n);

    double total_sum = 0.0;

    for (size_t i = 0; i < n; ++i) total_sum += max(regret_sum[offset+i], 0.0);
        
    if (total_sum > 0.0) {
        for (size_t i = 0; i < n; i++) output[i] = max(0.0, regret_sum[offset+i]) / total_sum;
    }

    else {
        double uniform = 1.0 / n;
        for (size_t i = 0; i < n; i++) output[i] = uniform;
    }

}

size_t InfoSets::sample_regret(const InfoKey& ikey, mt19937& rng, vector<double>& probs) const {

    get_regret_strategy(ikey ,probs);

    uniform_real_distribution<double> unif(0.0, 1.0);
    double r = unif(rng);
    double cum = 0.0;
    size_t idx = probs.size() - 1;  
    
    for (size_t i = 0; i < probs.size(); ++i) {
        cum += probs[i];
        if (r < cum) { idx = i; break; }
    }
    return idx;
}
vector<pair<Action, double>> InfoSets::get_average_strategy(const InfoKey& ikey) const{

    vector<pair<Action, double>> output;

    size_t offset = get_offset(ikey);
    size_t n = ikey.get_num_actions();
    output.resize(n);

    double sum = 0.0;
    for (size_t i = 0 ; i < n; ++i){
        sum += strategy_sum[offset+i];
    }

    if (sum <= 0.0) {
        double p = 1.0/double(n); 
        for (size_t i = 0; i < n; ++i){
            output[i] = {ikey.node.edges[i], p};
        }
    }

    for (size_t i = 0; i < n; ++i) {
        output[i] = {ikey.node.edges[i], strategy_sum[i] / sum};
    }

    return output;
}


void InfoSets::get_probs(const InfoKey& ikey, vector<double>& probs) const {
    size_t n = ikey.get_num_actions();
    probs.resize(n);
    get_regret_strategy(ikey, probs);      
}