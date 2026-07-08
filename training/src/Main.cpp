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

#include "cfr.h"
#include "game_state.h"
#include "info_set.h"

#include <filesystem>
namespace fs = std::filesystem;
using namespace std;

static void run_game(int epochs,
                     int iters_per_epoch, Abstraction abs) {
                        
    unique_ptr<GameState> init_state = make_unique<GameState>();
    CFR solver(12345u, 500000, 200 , std::move(init_state), abs);

    cout << "Did we init the CFR" << endl;

    for (int epoch = 0; epoch < epochs; ++epoch) {
        solver.train(iters_per_epoch);
        cout << "Finished epoch " << epoch << endl; 
    }
}


// static uint8_t parse_rank(char rc) {
//     rc = (char)std::toupper((unsigned char)rc);
//     switch (rc) {
//         case '2': return 0;
//         case '3': return 1;
//         case '4': return 2;
//         case '5': return 3;
//         case '6': return 4;
//         case '7': return 5;
//         case '8': return 6;
//         case '9': return 7;
//         case 'T': return 8;
//         case 'J': return 9;
//         case 'Q': return 10;
//         case 'K': return 11;
//         case 'A': return 12;
//         default: throw invalid_argument("Bad rank character");
//     }
// }

// static uint8_t parse_suit(char sc) {
//     switch (sc) {
//         case 'c': return 0;
//         case 'd': return 1;
//         case 'h': return 2;
//         case 's': return 3;
//         default: throw invalid_argument("Bad suit character");
//     }
// }

// static uint8_t string_to_card(char rc, char sc){
//     uint8_t suit_int = parse_suit(sc);
//     uint8_t rank_int = parse_rank(rc);

//     return


// }

// unordered_map<string, double> extract_preflop(unique_ptr<GameState> init_state ){

//     vector<string> suits{"c", "d"};
//     vector<string> ranks{"A", "K", "Q", "J", "T", "9", "8", "7", "6", "5", "4", "3", "2"};

//     for (size_t rk_0 = 0; rk_0 < ranks.size(); ++rk_0){
//         for (size_t rk_1  = rk_0; rk_1 < ranks.size(); ++rk_1){

//             //onsuit: 

//         }
//     }

//     for (string r



    

// }

int main(int argc, char** argv) {
    try {
        using clock = std::chrono::steady_clock;

        cout << "Started" << endl;
        fs::path exe = fs::weakly_canonical(fs::path(argv[0]));
        fs::path root = exe.parent_path().parent_path();
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

        run_game(epochs, iters_per_epoch, abs);
        auto t1 = clock::now();

        double seconds = std::chrono::duration<double>(t1 - t0).count();
        cout << "Number of traverses: " << epochs * iters_per_epoch;
        cout << fixed << setprecision(6) << "Elapsed: " << seconds << " seconds\n";

    } 
        catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
