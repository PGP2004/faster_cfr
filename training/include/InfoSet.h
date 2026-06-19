#pragma once
#include "GameState.h"

using std::string;
using std::vector;
using std::unordered_map;
using std::pair;
using std::mt19937;

//TODO: Review discounting

class InfoSet {
    
protected: 
    void make_actions(const GameState& state);

    //the fuck is this: abstract and concrete representatino of legal actions
    //at some point switch to not needing strings
    vector<pair<string, Action>> abs_and_concrete;

    vector<double> regret_sum; 
    vector<double> strategy_sum; 
    int last_t = 0;
    
    // static constexpr double positive_regret_exp = 1.5;
    // static constexpr double negative_regret_exp = 0.0;
    // static constexpr double strat_exp = 2.0;

public:

    explicit InfoSet(const GameState& state);
    InfoSet(const InfoSet&) = default;
    InfoSet(InfoSet&&) = default;
    InfoSet& operator=(const InfoSet&) = default;
    InfoSet& operator=(InfoSet&&) = default;
    
    void get_regret_strategy(vector<double>& output) const;

    void update_regret(const vector<double>& action_deltas, int t);

    void update_average_strategy(double reach_prob, vector<double>& cur_strat, int t);

    pair<Action, double> sample_regret_action(mt19937& rng, vector<double>& probabilities) const;
   
    void get_action_w_probs(vector<pair<Action, double>>& actions_out, vector<double>& probs_out) const;
    void update_last_t(int t);
    unordered_map<string, double> get_average_strategy() const;

};