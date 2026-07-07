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

unordered_map<string, double> extract_preflop(unique_ptr<GameState> init_state ){
    

}

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
