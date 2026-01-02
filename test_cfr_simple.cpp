#include "CFR.h"
#include "GameState.h"
#include <iostream>
#include <random>
using namespace std;

int main() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    auto init_state = make_unique<GameState>(board, hands);
    
    CFR solver(12345, std::move(init_state));
    
    try {
        solver.train(1);  // Just 1 iteration
        cout << "Success!" << endl;
    } catch (const exception& e) {
        cout << "Error: " << e.what() << endl;
    }
    
    return 0;
}
