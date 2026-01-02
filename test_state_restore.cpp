#include "GameState.h"
#include <iostream>
#include <random>
using namespace std;

int main() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    
    // Deal
    auto undo0 = state.apply_chance(rng);
    int pot_after_deal = state.get_pot();
    cout << "After deal: pot=" << pot_after_deal << endl;
    
    // Apply and undo multiple actions
    Action call = {2, 0};
    for (int i = 0; i < 5; i++) {
        auto undo = state.apply_action(call);
        cout << "After action " << i << ": pot=" << state.get_pot() << endl;
        state.undo_action(undo, call);
        cout << "After undo " << i << ": pot=" << state.get_pot() << endl;
        
        if (state.get_pot() != pot_after_deal) {
            cout << "ERROR: Pot not restored! Expected " << pot_after_deal << ", got " << state.get_pot() << endl;
            return 1;
        }
    }
    
    cout << "State restoration test passed!" << endl;
    return 0;
}
