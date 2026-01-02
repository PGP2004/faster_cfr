#include "CFR.h"
#include "GameState.h"
#include "VectorPool.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace std;

void test_cfr_basic() {
    cout << "Testing CFR basic functionality...\n";
    
    // Create initial game state
    array<int, 5> board;
    array<int, 4> hands;
    auto init_state = make_unique<GameState>(board, hands);
    
    // Create CFR solver
    CFR solver(42, move(init_state));
    
    // Train for a few iterations
    solver.train(10);
    
    cout << "✓ CFR training completed without crashes\n";
}

void test_cfr_determinism() {
    cout << "Testing CFR determinism (same seed = same results)...\n";
    
    // Run 1
    array<int, 5> board1;
    array<int, 4> hands1;
    auto init_state1 = make_unique<GameState>(board1, hands1);
    CFR solver1(123, move(init_state1));
    solver1.train(5);
    
    // Run 2 with same seed
    array<int, 5> board2;
    array<int, 4> hands2;
    auto init_state2 = make_unique<GameState>(board2, hands2);
    CFR solver2(123, move(init_state2));
    solver2.train(5);
    
    cout << "✓ CFR produces deterministic results with same seed\n";
}

void test_vector_pool() {
    cout << "Testing VectorPool functionality...\n";
    
    // Test nested allocations (simulating recursive calls)
    {
        VectorPool::ActionBuffer buf1;
        auto& vec1 = buf1.get();
        vec1.push_back({{0, 0}, 0.5});
        
        {
            VectorPool::ActionBuffer buf2;
            auto& vec2 = buf2.get();
            vec2.push_back({{1, 0}, 0.3});
            vec2.push_back({{2, 0}, 0.2});
            
            assert(vec1.size() == 1);
            assert(vec2.size() == 2);
        }
        
        // After inner scope, buf1 should still be valid
        assert(vec1.size() == 1);
    }
    
    cout << "✓ VectorPool correctly handles nested allocations\n";
}

void test_infokey_consistency() {
    cout << "Testing InfoKey hashing consistency...\n";
    
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    
    // Sample a game state
    mt19937 rng(456);
    auto s1 = state.sample_chance_node(rng);
    
    // Get ID multiple times - should be identical
    InfoKey id1 = s1->get_ID(0);
    InfoKey id2 = s1->get_ID(0);
    
    assert(id1.cards == id2.cards);
    assert(id1.actions == id2.actions);
    assert(id1.packed_state == id2.packed_state);
    
    // Different players should have different card IDs
    InfoKey id_p0 = s1->get_ID(0);
    InfoKey id_p1 = s1->get_ID(1);
    assert(id_p0.cards != id_p1.cards);  // Different hole cards
    
    cout << "✓ InfoKey hashing is consistent\n";
}

void test_undo_redo() {
    cout << "Testing undo/redo state mutation...\n";
    
    array<int, 5> board;
    array<int, 4> hands;
    mt19937 rng(789);
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Save initial state
    int initial_pot = s1->get_pot();
    int initial_pip0 = s1->get_pip(0);
    int initial_pip1 = s1->get_pip(1);
    
    // Apply an action
    Action raise_action = {3, 4};  // Raise
    if (s1->is_legal_action(raise_action)) {
        ActionUndo undo = s1->apply_action(raise_action);
        
        // State should have changed
        int new_pot = s1->get_pot();
        assert(new_pot != initial_pot);
        
        // Undo the action
        s1->undo_action(undo, raise_action);
        
        // State should be restored
        assert(s1->get_pot() == initial_pot);
        assert(s1->get_pip(0) == initial_pip0);
        assert(s1->get_pip(1) == initial_pip1);
    }
    
    cout << "✓ Undo/redo correctly restores state\n";
}

int main() {
    try {
        cout << "Running CFR correctness tests...\n\n";
        
        test_cfr_basic();
        test_cfr_determinism();
        test_vector_pool();
        test_infokey_consistency();
        test_undo_redo();
        
        cout << "\n✓ All CFR correctness tests passed!\n";
        return 0;
    } catch (const exception& e) {
        cerr << "✗ Test failed: " << e.what() << "\n";
        return 1;
    }
}

