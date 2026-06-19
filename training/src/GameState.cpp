#include "Utils.h"
#include "Packings.h"
#include "GameState.h"

#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>
#include <iostream>
#include <array>
#include <cstdint>
#include <random>
#include <utility>  

using namespace std;

using std::mt19937;



static void shuffle_and_deal(mt19937& rng, array<array<int,2>,2>& hands, array<int,5>& board) {

    //deck object created just once
    static array<int, 52> deck = []{
        array<int,52> d{};
        for (int i = 0; i < 52; i++) d[i] = i;
        return d;}();

    //shuffle deck
    for (int i = 0; i < 9; ++i) {
        uniform_int_distribution<int> dist(i, 51);
        int j = dist(rng);
        swap(deck[i], deck[j]);
    }

    for (int i = 0; i < 5; ++i) board[i] = deck[i];
    hands[0][0] = deck[5]; hands[0][1] = deck[6];
    hands[1][0] = deck[7]; hands[1][1] = deck[8];
}

GameState::GameState(): board{{-1,-1, -1, -1, -1}}, hands{{{-1,-1}, {-1,-1}}}, p1_win_share(-1), stacks{starting_stack, starting_stack},
 pips{0,0}, pot(0),street(0),active_player(0), last_action{-1,-1}, packed_actions{}, packed_cards{} {}

double GameState::get_p1_winshare() const{
    uint8_t r0[7], s0[7];
    uint8_t r1[7], s1[7];

    for (int i = 0; i < 5; ++i) {
        uint8_t c = (uint8_t) board[i];
        r0[i] = card_rank(c);  s0[i] = card_suit(c);
        r1[i] = card_rank(c);  s1[i] = card_suit(c);
    }

    uint8_t p0_card_1 = (uint8_t)hands[0][0], p0_card_2 = (uint8_t)hands[0][1];
    r0[5] = card_rank(p0_card_1); s0[5] = card_suit(p0_card_1);
    r0[6] = card_rank(p0_card_2); s0[6] = card_suit(p0_card_2);

    uint8_t p1_card_1 = (uint8_t)hands[1][0], p1_card_2 = (uint8_t)hands[1][1];
    r1[5] = card_rank(p1_card_1); s1[5] = card_suit(p1_card_1);
    r1[6] = card_rank(p1_card_2); s1[6] = card_suit(p1_card_2);

    uint32_t score0 = evaluate_raw(r0, s0, 7);
    uint32_t score1 = evaluate_raw(r1, s1, 7);

    if (score1 > score0) return 1.0;
    if (score1 < score0) return 0.0;
    return 0.5;
}

double GameState::get_reward(int player) const {

    if (!is_terminal_node()) throw logic_error("Cannot assign rewards from non-terminal state");
    if (player != 0 && player != 1) throw logic_error("The player index must be one of 1 or 0");

    double win_share = 0.0;

    if (last_action.type == 0){
        if (player == active_player) win_share = 1.0;
    }

    else{
        if (player == 1) win_share = p1_win_share;
        else win_share = 1.0 - p1_win_share;
    }

    double reward = (stacks[player] - starting_stack) + win_share * static_cast<double>(pot);

    return (stacks[player] - starting_stack) + win_share * static_cast<double>(pot);
}


bool GameState::is_legal_action(const Action& action) const {
  
    if (is_terminal_node() || is_chance_node()) return false;
    if (action.type < 0 || action.type > 3) return false;
         
    int to_call = pips[1-active_player] - pips[active_player];
    bool facing_bet = (to_call > 0);

    if (action.type == 0) return facing_bet && action.amt == 0; // fold
    if (action.type == 2) return facing_bet && action.amt == 0 && to_call <= stacks[active_player]; // call
    if (action.type == 1) return !facing_bet && action.amt == 0;// check

    if (action.type != 3) throw logic_error("Shoudl not get here");

    int cur_bet = max(pips[0], pips[1]);
    int min_raise_to = cur_bet + (facing_bet ? max(2, to_call) : 2);
    int max_raise_to = min(pips[0] + stacks[0], pips[1] + stacks[1]);

    return (action.amt >= min_raise_to) && (action.amt <= max_raise_to);
}


void GameState::write_action_undo(const Action& action, ActionUndo& undo) const {
    if (is_chance_node() || is_terminal_node())
        throw logic_error("cant call action on chance or terminal");

    int to_pay = 0;
    if (action.type == 2) { // call
        to_pay = pips[1 - active_player] - pips[active_player];
    } else if (action.type == 3) { 
        to_pay = action.amt - pips[active_player];
    }

    undo.old_to_pay = to_pay;
    undo.old_last_action = last_action;
    undo.old_street = street;
    undo.old_active_player = active_player;
}


int GameState::abs_id_from_action(const Action& a) const{
    if (a.type < 3) return a.type ;

    int inc = a.amt - pips[active_player];  

    if (inc == pot/2)   return 3;
    if (inc == pot)     return 4;
    if (inc == 2*pot)   return 5;

    throw logic_error("Raise amount not in abstraction");
}

void GameState::apply_action(const Action& action) {

    if (is_chance_node() || is_terminal_node()){ throw logic_error("cant call action on chance or terminal");}

    int abs_id = abs_id_from_action(action);
    packed_actions.push(abs_id);
      
    int to_pay = 0;
    if (action.type == 2) to_pay = pips[1 - active_player] - pips[active_player];
    else if (action.type == 3) to_pay = action.amt - pips[active_player];

    pips[active_player] += to_pay;
    pot += to_pay;
    stacks[active_player] += -to_pay;
    
    bool round_ended = false;

    if (last_action.type == 3 && action.type == 2) round_ended = true; //raise , call
    if (last_action.type == 1 && action.type == 1) round_ended = true; //check check

    last_action = action;
    active_player = 1 - active_player; 

    if (action.type == 0) { 
        street = 8;
    }

    else if (round_ended){
        street += 1;
        //pips cleaned in the chance round;;
    }
}

void GameState::undo_action(const ActionUndo& undo) {

    last_action = undo.old_last_action;
    street = undo.old_street; 
    active_player = undo.old_active_player;

    int to_pay = undo.old_to_pay;
    pips[active_player] += -to_pay; 
    pot += -to_pay;
    stacks[active_player] += to_pay;

    packed_actions.del();
}


void GameState::write_chance_undo(ChanceUndo& undo) const{

    if (!(is_chance_node())){ throw logic_error("cant call chance on non chance node");}
    undo.old_pips = pips;
    undo.old_stacks = stacks;
    undo.old_pot = pot;
    undo.old_active_player = active_player;
    undo.old_street = street;
    undo.old_last_action = last_action;
}

void GameState::apply_chance(mt19937& rng) {

    if (!is_chance_node()){ throw logic_error("Can only apply chance in a chance node");}

    pips = {0, 0};
    active_player = 0;
    last_action = {-1, -1};

    if (street == 0) { 
        shuffle_and_deal(rng, hands, board);
        p1_win_share = get_p1_winshare();

        active_player = 1;
        pot = 3;
        stacks[0] = starting_stack - 2;
        stacks[1] = starting_stack - 1;
        pips[0] = 2;
        pips[1] = 1;

        for (int i = 0; i < 2; i++){
            packed_cards[0].push(hands[0][i]);
            packed_cards[1].push(hands[1][i]);
        }
    }

    if (street == 2) { 
        active_player = 0;
        for (int i = 0; i < 3; i++){
            packed_cards[0].push(board[i]);
            packed_cards[1].push(board[i]);
        }
    }

    if (street == 4) { 
        active_player = 0;
        packed_cards[0].push(board[3]);
        packed_cards[1].push(board[3]);
    }

    if (street == 6) { 
        active_player = 0;
        packed_cards[0].push(board[4]);
        packed_cards[1].push(board[4]);
    }

    street += 1;
}

void GameState::undo_chance(const ChanceUndo& undo) {

    if (undo.old_street == 0){
        board.fill(-1);
        hands[0].fill(-1);
        hands[1].fill(-1);
        p1_win_share = -1;
    }
    
    int n = 0;
    if (undo.old_street == 0) n = 2;
    else if (undo.old_street == 2) n = 3;
    else if (undo.old_street == 4 || undo.old_street == 6) n = 1;

    for (int i = 0; i < n; i++){
        packed_cards[0].del();
        packed_cards[1].del();
    }

    pips = undo.old_pips;
    stacks = undo.old_stacks;
    pot = undo.old_pot;
    active_player = undo.old_active_player;
    street = undo.old_street;
    last_action = undo.old_last_action;
}
