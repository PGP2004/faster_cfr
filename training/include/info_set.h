#pragma once
#include "game_state.h"
#include <unordered_map>
#include <vector>
#include <string>
#include <utility>
#include <random>


//TODO: Review discounting

class InfoSet {
    
protected: 
    void make_actions(const GameState& state);

    //the fuck is this: abstract and concrete representatino of legal actions
    //at some point switch to not needing strings
    std::vector<std::pair<std::string, Action>> abs_and_concrete;
    std::vector<double> regret_sum; 
    std::vector<double> strategy_sum; 
    int last_t = 0;
    
public:

    explicit InfoSet(const GameState& state);
    InfoSet(const InfoSet&) = default;
    InfoSet(InfoSet&&) = default;
    InfoSet& operator=(const InfoSet&) = default;
    InfoSet& operator=(InfoSet&&) = default;
    
    void get_regret_strategy(vector<double>& output) const;

    void update_regret(const vector<double>& action_deltas, int t);

    void update_average_strategy(double reach_prob, std::vector<double>& cur_strat, int t);

    std::pair<Action, double> sample_regret_action(std::mt19937& rng, std::vector<double>& probabilities) const;
   
    void get_action_w_probs(std::vector<Action>&   actions_out, std::vector<double>& probs_out) const;
    void update_last_t(int t);

    std::unordered_map<string, double> get_average_strategy() const;

};