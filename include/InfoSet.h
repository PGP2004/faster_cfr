#pragma once
#include <random>
#include <unordered_map>
#include <string>
#include <vector>
#include <utility>
#include <random>
#include "GameState.h"

using std::string;
using std::vector;
using std::unordered_map;
using std::pair;
using std::mt19937;

class InfoSet {
    
protected: 
    void make_actions(const GameState& state);
    vector<pair<string, Action>> abs_and_concrete;
    vector<double> regret_sum; 
    vector<double> strategy_sum; 
    int last_t;

    static constexpr double regret_discount_exp = 1.5;
    static constexpr double strat_discount_exp = 2.0;


public:
    explicit InfoSet(const GameState& state);
    vector<double> get_regret_strategy() const;
    pair<Action, double> sample_regret_action(mt19937& rng) const;

    void update_regret(const vector<double>& deltas, int t);
    void update_average_strategy(double reach_prob, int t);
    void update_last_t(int t);

    vector<pair<Action, double>> get_action_w_probs() const;
    unordered_map<string, double> get_average_strategy() const;

};