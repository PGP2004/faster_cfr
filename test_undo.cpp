#include "GameState.h"
#include <iostream>
#include <random>
using namespace std;

int main() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    
    // Deal cards
    auto chance_undo = state.apply_chance(rng);
    cout << "After deal - pot: " << state.get_pot() << ", P0 pip: " << state.get_pip(0) << ", P1 pip: " << state.get_pip(1) << endl;
    
    // P1 calls
    Action call_action = {2, 0};
    auto undo1 = state.apply_action(call_action);
    cout << "After P1 call - pot: " << state.get_pot() << ", P0 pip: " << state.get_pip(0) << ", P1 pip: " << state.get_pip(1) << endl;
    
    // Undo
    state.undo_action(undo1, call_action);
    cout << "After undo - pot: " << state.get_pot() << ", P0 pip: " << state.get_pip(0) << ", P1 pip: " << state.get_pip(1) << endl;
    
    // Try again
    auto undo2 = state.apply_action(call_action);
    cout << "After P1 call again - pot: " << state.get_pot() << ", P0 pip: " << state.get_pip(0) << ", P1 pip: " << state.get_pip(1) << endl;
    
    return 0;
}
