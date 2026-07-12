#include <algorithm>
#include <chrono>
#include <exception>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cmath>
#include <filesystem>
#include <fstream>

#include "cfr.h"
#include "game_state.h"
#include "info_set.h"

namespace fs = std::filesystem;
using namespace std;



unordered_map<string, double> extract_preflop(CFR cfr_run){

    vector<char> suits{'c', 'd'};
    vector<char> ranks{'A', 'K', 'Q', 'J', 'T', '9', '8', '7', '6', '5', '4', '3', '2'};
    unordered_map<string, double> output;

    static const array<uint8_t, 1> preflop_counts = {2};
    static Indexer preflop_indexer(preflop_counts.size(), preflop_counts.data());
    array<uint8_t,2> hand;

    for (size_t rk_0 = 0; rk_0 < ranks.size(); ++rk_0){
        for (size_t rk_1  = 0; rk_1 <= rk_0; ++rk_1){

            //same suit case, ranks cant be the same
            if (rk_0 != rk_1){    
                card_t c0 = string_to_card(ranks[rk_0], 'c');
                card_t c1 = string_to_card(ranks[rk_1], 'c');

                hand[0] = c0; hand[1] = c1;
                int hand_id = static_cast<int>(hand_index_last(&preflop_indexer.h, hand.data()));
                InfoKey ikey{0, hand_id, {}};
                double bet_prob = cfr_run.get_action_prob(1, ikey, "pot");

                string s_key{ranks[rk_1], ranks[rk_0]};
                s_key += 's';  
                output[s_key] = bet_prob;
            }

            //offsuit case
            card_t c0 = string_to_card(ranks[rk_0], 'c');
            card_t c1 = string_to_card(ranks[rk_1], 'd');

            hand[0] = c0; hand[1] = c1;
            int hand_id = static_cast<int>(hand_index_last(&preflop_indexer.h, hand.data()));

            InfoKey ikey{0, hand_id, {}};
            double bet_prob = cfr_run.get_action_prob(1, ikey, "pot");

            string o_key{ranks[rk_1], ranks[rk_0]};
            o_key += 'o';            
            output[o_key] = bet_prob;
        }
    }
    return output;
}


static  vector<unordered_map<string, double>> run_game(Abstraction abs) {
                        
    GameState init_state = GameState();
    CFR solver(12345u, 500000, 200 , init_state, abs);

    vector<int> chk_pts;

    for (int i = 16; i < 22; ++i){
        chk_pts.push_back(pow(2,i));
    }

    vector<unordered_map<string, double>> chkpt_ranges;

    int last_iter = 0;
    for (size_t i = 0; i < chk_pts.size(); ++i){

        int target_iter = chk_pts[i];
        solver.train(target_iter - last_iter, last_iter);
        cout << "finished training to iter " << target_iter << endl;
        unordered_map<string,double> preflop_range = extract_preflop(solver);
        chkpt_ranges.push_back(preflop_range);
        cout << "extracted preflop range for iter: " << target_iter << endl;
        cout << "-------------------" << endl;
        last_iter = target_iter;
    }
    return chkpt_ranges;
}


//Claude written csv saver. TODO: write less sloppy one
void write_to_csvs(const vector<unordered_map<string, double>>& ckpt_ranges,
                   const vector<int>& iters, const string& path) {

    std::ofstream out(path);
    if (!out) { std::cerr << "could not open " << path << "\n"; return; }

    out << "iter,hand,pot_prob\n";
    for (size_t c = 0; c < ckpt_ranges.size(); ++c) {
        int iter = (c < iters.size()) ? iters[c] : static_cast<int>(c);
        for (const auto& [hand, prob] : ckpt_ranges[c]) {
            out << iter << "," << hand << ",";
            if (prob < 0.0) out << "";        // failed lookup -> NaN in pandas
            else            out << prob;
            out << "\n";
        }
    }
}

int main(int argc, char** argv) {
    try {
        using clock = std::chrono::steady_clock;

        cout << "Started" << endl;
        fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
        fs::path root = exe.parent_path().parent_path().parent_path().parent_path();
        cout << "The root is: " << root.string() << endl;
        fs::path storage = root / "clustering/storage";

        string flop_path = (storage / "flop_assignments").string();
        string turn_path = (storage / "turn_assignments" ).string();
        string river_path = (storage / "river_strengths" ).string();
        //This is 100 buckets for the river, fix later. Right now I only store the like strenths and dont do the bucketing yet
        //should j change some stuff in the make_turn_clusters file to make_river_clusters file

        Abstraction abs(flop_path, turn_path, river_path);
        cout << "DID we fnish loading the abstraction" << endl;
        int epochs = 100;
        int iters_per_epoch = 1000;

        auto t0 = clock::now();
        vector<unordered_map<string, double>> ckpt_ranges = run_game(abs);
        auto t1 = clock::now();
        double seconds = std::chrono::duration<double>(t1 - t0).count();
        cout << fixed << setprecision(6) << "Elapsed: " << seconds << " seconds\n";


        vector<int> ckpt_iters;
        for (int i = 16; i < 22; ++i){
            ckpt_iters.push_back(pow(2,i));
        }


        write_to_csvs(ckpt_ranges, ckpt_iters, "preflop_checkpoints.csv");

    } 
        catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
