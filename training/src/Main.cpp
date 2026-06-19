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

#include "CFR.h"
#include "GameState.h"
#include "InfoSet.h"


using namespace std;

static void run_game(int epochs,
                     int iters_per_epoch) {

    bool print_infosets = true;

    array<int, 5> board_buffer;
    array<int, 4> hands_buffer;

    unique_ptr<GameState> init_state = make_unique<GameState>();
    CFR solver(12345u, 500000, 200 , std::move(init_state));

    for (int epoch = 0; epoch < epochs; ++epoch) {
        solver.train(iters_per_epoch);
    }
}

int main() {
    try {
        using clock = std::chrono::steady_clock;

        int epochs = 10;
        int iters_per_epoch = 100;

        auto t0 = clock::now();

        for (int epoch = 0; epoch < epochs; ++epoch){
            run_game(epochs, iters_per_epoch);
            cout << "Hello" << endl;
        }

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
