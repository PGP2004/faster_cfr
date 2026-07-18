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
#include "info_sets.h"
#include "action_tree.h"

namespace fs = std::filesystem;
using namespace std;

int main(int argc, char** argv) {
    try {
        using clock = std::chrono::steady_clock;

        fs::path exe  = fs::weakly_canonical(fs::path(argv[0]));
        fs::path root = exe.parent_path().parent_path().parent_path().parent_path();
        fs::path storage = root / "clustering/storage";

        Abstraction abs((storage / "flop_assignments").string(),
                        (storage / "turn_assignments").string(),
                        (storage / "river_strengths").string());


        GameState init_state;
        ActionTree at(init_state);

        cout << "Do we get to here" << endl;
        //  CFR(uint32_t seed, GameState init_game_state, Abstraction& abstraction, ActionTree& action_tree);
        CFR cfr(init_state, abs, at);

        int iters = 100000;

        auto t0 = clock::now();
        cfr.train(iters, 0);
        double total = std::chrono::duration<double>(clock::now() - t0).count();
        cout << "Runtime for " << iters << " iterations is: " << total << " seconds" << endl;

    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}

