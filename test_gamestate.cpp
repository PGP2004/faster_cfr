#include "GameState.h"
#include "Utils.h"
#include <iostream>
#include <cassert>
#include <random>
#include <iomanip>

using namespace std;

// Helper function to print test results
void print_test(const string& name, bool passed) {
    cout << (passed ? "✓ " : "✗ ") << name << endl;
    if (!passed) {
        cerr << "  FAILED!" << endl;
        exit(1);
    }
}

// Test 1: Initial state construction
void test_initial_state() {
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    
    assert(state.is_chance_node() && "Initial state should be a chance node (street 0)");
    assert(!state.is_terminal() && "Initial state should not be terminal");
    assert(state.get_pot() == 0 && "Initial pot should be 0");
    assert(state.get_active_player() == 0 && "Initial active player should be 0");
    
    print_test("Initial state construction", true);
}

// Test 2: Chance node progression (dealing cards and blinds)
void test_chance_nodes() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    
    // Street 0 -> Street 1 (deal cards, post blinds)
    assert(state.is_chance_node() && "Street 0 should be chance node");
    auto s1 = state.sample_chance_node(rng);
    
    assert(s1->get_pot() == 3 && "Pot should be 3 after blinds (2+1)");
    assert(s1->get_pip(0) == 2 && "Player 0 pip should be 2 (small blind)");
    assert(s1->get_pip(1) == 1 && "Player 1 pip should be 1 (big blind)");
    assert(s1->get_active_player() == 1 && "Player 1 should be active (SB acts first)");
    assert(!s1->is_chance_node() && "Street 1 should be betting round");
    
    // Verify cards were dealt
    auto hand0 = s1->get_hand(0);
    auto hand1 = s1->get_hand(1);
    assert(hand0[0] >= 0 && hand0[0] < 52 && "Player 0 card 1 should be valid");
    assert(hand0[1] >= 0 && hand0[1] < 52 && "Player 0 card 2 should be valid");
    assert(hand1[0] >= 0 && hand1[0] < 52 && "Player 1 card 1 should be valid");
    assert(hand1[1] >= 0 && hand1[1] < 52 && "Player 1 card 2 should be valid");
    
    // Verify board is empty before flop
    auto board_preflop = s1->get_board();
    assert(board_preflop[0] == -1 && "Board should be empty preflop");
    
    print_test("Chance nodes (card dealing and blinds)", true);
}

// Test 3: Legal action checking
void test_legal_actions() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Player 1 facing bet (pip 1 vs 2)
    assert(s1->is_legal_action({0, 0}) && "Fold should be legal when facing bet");
    assert(s1->is_legal_action({2, 0}) && "Call should be legal when facing bet");
    assert(!s1->is_legal_action({1, 0}) && "Check should NOT be legal when facing bet");
    
    // Raises should have proper sizing
    // Minimum raise to 4 (current bet 2 + min raise 2)
    assert(s1->is_legal_action({3, 4}) && "Minimum raise to 4 should be legal");
    assert(s1->is_legal_action({3, 5}) && "Raise to 5 should be legal");
    assert(!s1->is_legal_action({3, 3}) && "Raise to 3 should be too small");
    
    // Invalid action types
    assert(!s1->is_legal_action({-1, 0}) && "Negative action type should be illegal");
    assert(!s1->is_legal_action({4, 0}) && "Invalid action type should be illegal");
    
    print_test("Legal action checking", true);
}

// Test 4: Action execution and state transitions
void test_action_execution() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Player 1 calls
    auto s2 = s1->next_game_state({2, 0}); // Call
    assert(s2->get_pot() == 4 && "Pot should be 4 after call (2+2)");
    assert(s2->get_pip(1) == 2 && "Player 1 pip should be 2 after calling to 2");
    assert(s2->get_active_player() == 0 && "Active player should switch to 0");
    
    print_test("Action execution and state transitions", true);
}

// Test 5: Betting round completion
void test_betting_round() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Player 1 calls, Player 0 checks, Player 1 checks -> round ends
    auto s2 = s1->next_game_state({2, 0}); // P1 calls
    auto s3 = s2->next_game_state({1, 0}); // P0 checks
    auto s4 = s3->next_game_state({1, 0}); // P1 checks (check-check ends round)
    
    assert(s4->is_chance_node() && "Should be chance node after betting round ends");
    assert(s4->get_pip(0) == 0 && "Pips should reset after round");
    assert(s4->get_pip(1) == 0 && "Pips should reset after round");
    
    // Sample flop
    auto s5 = s4->sample_chance_node(rng);
    auto board_flop = s5->get_board();
    assert(board_flop[0] != -1 && "First flop card should be dealt");
    assert(board_flop[1] != -1 && "Second flop card should be dealt");
    assert(board_flop[2] != -1 && "Third flop card should be dealt");
    assert(board_flop[3] == -1 && "Turn card should not be dealt yet");
    
    print_test("Betting round completion", true);
}

// Test 6: Fold to terminal
void test_fold() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Player 1 folds
    auto s2 = s1->next_game_state({0, 0});
    
    assert(s2->is_terminal() && "State should be terminal after fold");
    
    // Player 0 (the folder's opponent) should win
    double reward_p0 = s2->get_rewards(0);
    double reward_p1 = s2->get_rewards(1);
    
    // Player 0 wins pot (3 chips) minus their blind (2), net +1
    // Player 1 loses their blind (1), net -1
    assert(reward_p0 == 1 && "Player 0 should win 1 chip");
    assert(reward_p1 == -1 && "Player 1 should lose 1 chip");
    
    print_test("Fold to terminal state", true);
}

// Test 7: Raise sizing
void test_raise_sizing() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    int pot = s1->get_pot(); // 3
    int my_pip = s1->get_pip(1); // 1
    
    // Pot-sized raise: call to 2, then raise by pot (3) = pip of 2+3 = 5
    // Increment is 5-1=4, pot is 3... wait that doesn't match
    // Let's use 2x pot: call to 2, then raise by 2*pot (6) = pip of 2+6 = 8
    // Increment is 8-1=7, but 2*pot = 6...
    
    // Actually, pot-sized from current pip: my_pip + pot = 1 + 3 = 4
    auto s2 = s1->next_game_state({3, my_pip + pot});
    assert(s2->get_pot() == 6 && "Pot should be 6 (3 + 3 new chips)");
    assert(s2->get_pip(1) == 4 && "Raiser pip should be 4");
    assert(s2->get_active_player() == 0 && "Active player should switch");
    
    print_test("Raise sizing", true);
}

// Test 8: Check-check progression
void test_check_check() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Get past preflop
    auto s2 = s1->next_game_state({2, 0}); // P1 calls
    auto s3 = s2->next_game_state({1, 0}); // P0 checks
    auto s4 = s3->next_game_state({1, 0}); // P1 checks - round ends
    auto s5 = s4->sample_chance_node(rng); // Deal flop
    
    // Both players check
    auto s6 = s5->next_game_state({1, 0}); // P0 checks
    assert(s6->get_active_player() == 1 && "Active player should be 1");
    
    auto s7 = s6->next_game_state({1, 0}); // P1 checks
    assert(s7->is_chance_node() && "Should advance to next street after check-check");
    
    print_test("Check-check progression", true);
}

// Test 9: Raise-call progression  
void test_raise_call() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Get past preflop
    auto s2 = s1->next_game_state({2, 0}); // P1 calls
    auto s3 = s2->next_game_state({1, 0}); // P0 checks
    auto s4 = s3->next_game_state({1, 0}); // P1 checks - round ends
    auto s5 = s4->sample_chance_node(rng); // Deal flop
    
    int pot = s5->get_pot();
    int p0_pip = s5->get_pip(0);
    
    // P0 raises by pot amount
    auto s6 = s5->next_game_state({3, p0_pip + pot});
    assert(!s6->is_chance_node() && "Should not be chance node after raise");
    
    // P1 calls
    auto s7 = s6->next_game_state({2, 0});
    assert(s7->is_chance_node() && "Should be chance node after raise-call");
    
    print_test("Raise-call progression", true);
}

// Test 10: Invalid actions throw exceptions
void test_invalid_actions() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    bool threw = false;
    try {
        s1->next_game_state({1, 0}); // Check when facing bet - illegal
    } catch (const invalid_argument&) {
        threw = true;
    }
    assert(threw && "Should throw on illegal action");
    
    // Can't take action on chance node
    threw = false;
    try {
        state.next_game_state({1, 0}); // Try to act on chance node
    } catch (const logic_error&) {
        threw = true;
    }
    assert(threw && "Should throw when acting on chance node");
    
    print_test("Invalid actions throw exceptions", true);
}

// Test 11: Full game to showdown
void test_full_game_showdown() {
    mt19937 rng(42);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    
    // Deal cards
    auto s1 = state.sample_chance_node(rng);
    
    // Preflop: call, check, check
    auto s2 = s1->next_game_state({2, 0});
    auto s3 = s2->next_game_state({1, 0});
    auto s3b = s3->next_game_state({1, 0});
    
    // Flop
    auto s4 = s3b->sample_chance_node(rng);
    auto s5 = s4->next_game_state({1, 0}); // check
    auto s6 = s5->next_game_state({1, 0}); // check
    
    // Turn
    auto s7 = s6->sample_chance_node(rng);
    auto s8 = s7->next_game_state({1, 0}); // check
    auto s9 = s8->next_game_state({1, 0}); // check
    
    // River
    auto s10 = s9->sample_chance_node(rng);
    auto s11 = s10->next_game_state({1, 0}); // check
    auto s12 = s11->next_game_state({1, 0}); // check
    
    assert(s12->is_terminal() && "Game should be terminal after river check-check");
    
    // Check rewards sum to zero (conservation)
    double r0 = s12->get_rewards(0);
    double r1 = s12->get_rewards(1);
    assert(abs(r0 + r1) < 0.001 && "Rewards should sum to zero");
    
    print_test("Full game to showdown", true);
}

// Test 12: Player ID generation
void test_player_ids() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    auto id0 = s1->get_ID(0);
    auto id1 = s1->get_ID(1);
    
    // Players have different cards, so card IDs should be different
    assert(id0.cards != id1.cards && "Players should have different card IDs");
    
    // Same action history
    assert(id0.actions == id1.actions && "Players should have same action ID");
    
    // packed_state includes pot and pips from player's perspective
    // Players will have different packed_state if pips are different (my_pip vs opp_pip are swapped)
    
    // After an action, action IDs should change
    auto s2 = s1->next_game_state({2, 0});
    auto id0_after = s2->get_ID(0);
    assert(id0_after.actions != id0.actions && "Action ID should change after action");
    
    print_test("Player ID generation", true);
}

// Test 13: Stack updates
void test_stack_updates() {
    mt19937 rng(12345);
    array<int, 5> board;
    array<int, 4> hands;
    GameState state(board, hands);
    auto s1 = state.sample_chance_node(rng);
    
    // Player 1 started with 400, posted 1 blind
    // After calling 1 more, should have 398
    auto s2 = s1->next_game_state({2, 0}); // Call 1 chip
    
    // Can't directly check stacks, but can verify through large raises
    int pot = s2->get_pot();
    
    // Player 0 should be able to bet their remaining stack
    // They have 400 - 2 (blind) = 398 left
    assert(s2->is_legal_action({3, 398}) && "Should be able to bet remaining stack");
    
    print_test("Stack updates", true);
}

int main() {
    cout << "Running GameState tests...\n" << endl;
    
    test_initial_state();
    test_chance_nodes();
    test_legal_actions();
    test_action_execution();
    test_betting_round();
    test_fold();
    test_raise_sizing();
    test_check_check();
    test_raise_call();
    test_invalid_actions();
    test_full_game_showdown();
    test_player_ids();
    test_stack_updates();
    
    cout << "\n✓ All tests passed!" << endl;
    return 0;
}

