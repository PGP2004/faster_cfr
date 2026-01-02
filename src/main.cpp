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


static void print_infosets(const CFR solver) {
    const auto& all = solver.view_infosets();

    cout << fixed << setprecision(4);

    for (int p = 0; p < 2; ++p) {
        cout << "=== Player " << p << " infosets ===\n";

        vector<string> ids;
        ids.reserve(all[p].size());
        for (const auto& kv : all[p]) ids.push_back(kv.first);
        sort(ids.begin(), ids.end());

        for (const string& infoset_id : ids) {
            const InfoSet& iset = all[p].at(infoset_id);
            unordered_map<string, double> strat = iset.get_average_strategy();

            cout << infoset_id << "  ";

            vector<pair<string,double>> items(strat.begin(), strat.end());
            sort(items.begin(), items.end(),
                 [](const auto& a, const auto& b){ return a.first < b.first; });

            bool first = true;
            for (const auto& [action, prob] : items) {
                if (!first) cout << ", ";
                first = false;
                cout << action << ":" << prob;
            }
            cout << "\n";
        }
        cout << "\n";
    }
}


static void run_game(int epochs,
                     int iters_per_epoch) {

    bool print_infosets = true;

    array<int, 5> board_buffer;
    array<int, 4> hands_buffer;

    unique_ptr<GameState> init_state = make_unique<GameState>(board_buffer, hands_buffer);
    CFR solver(12345u, std::move(init_state));

    for (int epoch = 0; epoch < epochs; ++epoch) {
        solver.train(iters_per_epoch);
    }

}

int main() {
    try {
        using clock = std::chrono::steady_clock;

        int epochs = 1;
        int iters_per_epoch = 20000;

        //Running Texas HoldEm with the dumbest abstraction possible.
        //will not converge in reasonable time frame
        auto t0 = clock::now();
        run_game(epochs, iters_per_epoch);
        auto t1 = clock::now();

        double seconds = std::chrono::duration<double>(t1 - t0).count();
        cout << fixed << setprecision(6) << "Elapsed: " << seconds << "\n";

    } 
        catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}