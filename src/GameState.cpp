#include "Utils.h"
#include "Packings.h"
#include "GameState.h"
#include "InfoKey.h"
#include "Profiler.h"
#include <algorithm>
#include <string>
#include <stdexcept>
#include <memory>

using namespace std;

// Streets:
// 0: Deal hole cards
// 1: Preflop betting
// 2: Deal flop
// 3: Flop betting
// 4: Deal turn
// 5: Turn betting
// 6: Deal river
// 7: River betting
// 8: Terminal
//
// Action = (type, amt)
// type: fold=0, check=1, call=2, raise=3

// Map concrete Action -> packed abstract id in [0,7].
// Must match your InfoSet candidate list exactly.
// Example mapping:
// 0 fold, 1 check ,2 call, 3 half_pot, 4 pot, 5 2xpot


//THIS FUNCIONT NEEXS OT BE WORKED ON.,, not right as is
static int abs_id_from_action(const GameState& st, const Action& a) {
    if (a.type < 3) return a.type ;

    int p = st.get_pot();

    // raise-to -> increment
    int inc = a.amt - st.get_pip(st.get_active_player());  // add get_pip(int)

    if (inc == p/2)   return 3;
    if (inc == p)     return 4;
    if (inc == 2*p)   return 5;

    throw logic_error("Raise amount not in abstraction");
}


void GameState::start_game(mt19937& rng, array<int, 5>& board, array<int, 4>& hands) {
    PROFILE_FUNCTION();
    static array<int, 52> deck = []{
        array<int, 52> a{};
        for (int i = 0; i < 52; ++i) a[i] = i;
        return a;
    }();

    for (int i = 0; i < 9; ++i) {
        uniform_int_distribution<int> dist(i, 51);
        int j = dist(rng);
        swap(deck[i], deck[j]);
    }

    for (int i = 0; i < 5; ++i) board[i] = deck[i];
    for (int i = 0; i < 4; ++i) hands[i] = deck[5 + i];
}

GameState::GameState(array<int, 5>& cur_board_ref, array<int, 4>& cur_hands_ref)
    : board_ref(cur_board_ref),
      hands_ref(cur_hands_ref),
      stacks({starting_stack, starting_stack}),
      pips({0, 0}),
      pot(0),
      street(0),
      active_player(0),
      last_action({-1, -1}),
      packed_actions(),
      packed_cards() {
    }

GameState::GameState(array<int, 5>& cur_board_ref,
                     array<int, 4>& cur_hands_ref,
                     array<int, 2> cur_stacks,
                     array<int, 2> cur_pips,
                     int cur_pot,
                     int cur_street,
                     int cur_active_player,
                     Action cur_last_action,
                     PackedActions cur_packed_actions,
                     array<PackedCards, 2> cur_packed_cards)
    : board_ref(cur_board_ref),
      hands_ref(cur_hands_ref),
      stacks(cur_stacks),
      pips(cur_pips),
      pot(cur_pot),
      street(cur_street),
      active_player(cur_active_player),
      last_action(cur_last_action),
      packed_actions(cur_packed_actions),
      packed_cards(cur_packed_cards) {
    }

bool GameState::is_legal_action(const Action& action) const {
    PROFILE_FUNCTION();
    if (street % 2 == 0) return false;
    if (action.type < 0 || action.type > 3) return false;

    bool facing_bet = (pips[active_player] < pips[1 - active_player]);

    if (action.type == 0 || action.type == 2) return facing_bet && action.amt == 0;
    if (action.type == 1) return !facing_bet && action.amt == 0;

    int cur_bet = max(pips[0], pips[1]);
    int last_raise_size = abs(pips[0] - pips[1]);              
    int min_raise_to = cur_bet + max(2, last_raise_size);     
    int max_raise_to = min(pips[0] + stacks[0], pips[1] + stacks[1]);

    if (action.type == 3) {
        return (action.amt >= min_raise_to) && (action.amt <= max_raise_to);
    }

    return false;
}

void GameState::add_legal_actions(int my_pip, int p, vector<pair<string, Action>>& abs_and_concrete) const {
    PROFILE_FUNCTION();
    if(street % 2 == 0) return;

    bool facing_bet = (pips[active_player] < pips[1 - active_player]);
    if(facing_bet){
        abs_and_concrete.push_back({"fold", {0, 0}});
        abs_and_concrete.push_back({"call", {2, 0}});
    } else {
        abs_and_concrete.push_back({"check", {1, 0}});
    }
    int cur_bet = max(pips[0], pips[1]);
    int last_raise_size = abs(pips[0] - pips[1]);              
    int min_raise_to = cur_bet + max(2, last_raise_size);     
    int max_raise_to = min(pips[0] + stacks[0], pips[1] + stacks[1]);

    int half = p / 2;
    int full = p;
    int two  = 2 * p;
    
    if(my_pip + half >= min_raise_to && my_pip + half <= max_raise_to){
        abs_and_concrete.push_back({"half_pot", {3, my_pip + half}});
    }
    if(my_pip + full >= min_raise_to && my_pip + full <= max_raise_to){
        abs_and_concrete.push_back({"full_pot", {3, my_pip + full}});
    }
    if(my_pip + two >= min_raise_to && my_pip + two <= max_raise_to){
        abs_and_concrete.push_back({"2x_pot", {3, my_pip + two}});
    }

    // vector<pair<string, Action>> candidates = {
    //     {"fold",     {0, 0}},
    //     {"check",    {1, 0}},
    //     {"call",     {2, 0}},
    //     {"half_pot", {3, my_pip + half}},
    //     {"full_pot", {3, my_pip + full}},
    //     {"2x_pot",   {3, my_pip + two}},
    // };

    // for (auto &na : candidates) {
    //     if (state.is_legal_action(na.second)) abs_and_concrete.push_back(na);
    // }
}

// Inline functions moved to header for performance

ActionUndo GameState::apply_action(const Action& action) {
    PROFILE_FUNCTION();
    
    // Save undo information
    ActionUndo undo;
    undo.old_last_action = last_action;
    undo.old_pips = pips;
    undo.old_street = street;
    undo.acting_player = active_player;  // Save who is acting
    
    // Calculate abstract action ID BEFORE modifying state
    undo.abs_id = abs_id_from_action(*this, action);
    
    // Calculate payment
    int to_pay = 0;
    if (action.type == 2) {  // Call
        to_pay = pips[1 - active_player] - pips[active_player];
    } else if (action.type == 3) {  // Raise
        to_pay = action.amt - pips[active_player];
    }
    undo.to_pay = to_pay;
    
    // Apply payment
    pips[active_player] += to_pay;
    stacks[active_player] -= to_pay;
    pot += to_pay;
    
    // Update packed actions
    packed_actions.push(undo.abs_id);
    
    // Check if round ended
    bool round_ended = false;
    if (last_action.type == 0) {
        round_ended = true;
    } else if (last_action.type != -1) {
        int a = last_action.type;
        int b = action.type;
        round_ended = (a == 1 && b == 1) || (a == 3 && b == 2);
    }
    undo.round_ended = round_ended;
    
    // Apply state changes
    if (action.type == 0) {  // Fold ends game
        street = 8;
        active_player = 1 - active_player;
    } else if (round_ended) {
        pips = {0, 0};
        active_player = 0;
        last_action = {-1, -1};
        street += 1;
    } else {
        active_player = 1 - active_player;
        last_action = action;
    }
    
    return undo;
}

void GameState::undo_action(const ActionUndo& undo_info, const Action& action) {
    PROFILE_FUNCTION();
    
    // Restore street (handles fold case where street jumped to 8)
    street = undo_info.old_street;
    
    // Reverse payment (do this BEFORE changing active_player)
    stacks[undo_info.acting_player] += undo_info.to_pay;
    pot -= undo_info.to_pay;
    
    // Reverse state changes based on what happened
    if (undo_info.round_ended) {
        // Round had ended, we were at street+1, player 0
        active_player = undo_info.acting_player;
    } else {
        // Normal action, just flip player
        active_player = 1 - active_player;
    }
    
    // Restore pips (handles both normal and round-end cases)
    pips = undo_info.old_pips;
    
    // Restore last action
    last_action = undo_info.old_last_action;
    
    // Reverse packed actions
    packed_actions.len--;
}

ChanceUndo GameState::apply_chance(std::mt19937& rng) {
    PROFILE_FUNCTION();
    
    // Save undo information
    ChanceUndo undo;
    undo.old_stacks = stacks;
    undo.old_pips = pips;
    undo.old_pot = pot;
    
    // Save cards that will be overwritten
    if (street == 0) {
        for (int i = 0; i < 5; ++i) undo.old_cards[i] = board_ref[i];
        for (int i = 0; i < 4; ++i) undo.old_cards[5 + i] = hands_ref[i];
    }
    // For other streets, we don't overwrite cards, just reveal them
    
    // Apply chance node logic (from sample_chance_node)
    pips = {0, 0};
    street += 1;
    active_player = 0;
    last_action = {-1, -1};
    
    if (street == 1) {  // Deal cards and post blinds
        start_game(rng, board_ref, hands_ref);
        active_player = 1;
        pot += 3;
        stacks[0] -= 2;
        stacks[1] -= 1;
        pips[0] = 2;
        pips[1] = 1;
        
        // Pack hole cards
        packed_cards = {PackedCards{}, PackedCards{}};
        packed_cards[0].push(hands_ref[0]);
        packed_cards[0].push(hands_ref[1]);
        packed_cards[1].push(hands_ref[2]);
        packed_cards[1].push(hands_ref[3]);
    }
    
    // Reveal board cards
    if (street == 3) {  // Flop
        for (int i = 0; i < 3; ++i) {
            packed_cards[0].push(board_ref[i]);
            packed_cards[1].push(board_ref[i]);
        }
    }
    if (street == 5) {  // Turn
        packed_cards[0].push(board_ref[3]);
        packed_cards[1].push(board_ref[3]);
    }
    if (street == 7) {  // River
        packed_cards[0].push(board_ref[4]);
        packed_cards[1].push(board_ref[4]);
    }
    
    return undo;
}

void GameState::undo_chance(const ChanceUndo& undo_info) {
    PROFILE_FUNCTION();
    
    // Restore street
    street -= 1;
    
    // Restore stacks, pips, pot
    stacks = undo_info.old_stacks;
    pips = undo_info.old_pips;
    pot = undo_info.old_pot;
    
    // Restore cards if we were at initial deal
    if (street == 0) {
        for (int i = 0; i < 5; ++i) board_ref[i] = undo_info.old_cards[i];
        for (int i = 0; i < 4; ++i) hands_ref[i] = undo_info.old_cards[5 + i];
        packed_cards = {PackedCards{}, PackedCards{}};
    } else {
        // Remove revealed cards from packed_cards
        int num_to_remove = (street == 2) ? 3 : 1;  // Flop reveals 3, turn/river reveal 1
        for (int i = 0; i < num_to_remove; ++i) {
            packed_cards[0].len--;
            packed_cards[1].len--;
        }
    }
    
    // Restore active player and last action
    active_player = (street == 0) ? 0 : (street % 2 == 1 ? 0 : 0);
    last_action = {-1, -1};
}

unique_ptr<GameState> GameState::next_game_state(const Action& action) const {
    PROFILE_FUNCTION();
    if (is_chance_node() || is_terminal()) {
        throw logic_error("Not a valid street to do an action on");
    }
    if (!is_legal_action(action)) {
        throw invalid_argument("Illegal action");
    }

    array<int, 2> new_stacks = stacks;
    array<int, 2> new_pips = pips;
    int new_pot = pot;
    int new_street = street;
    int new_active_player = 1 - active_player;
    Action new_last_action = action;

    PackedActions new_packed_actions = packed_actions;
    new_packed_actions.push(abs_id_from_action(*this, action));

    array<PackedCards, 2> new_packed_cards = packed_cards;

    int to_pay = 0;
    if (action.type == 2) {
        to_pay = pips[1 - active_player] - pips[active_player];
    } else if (action.type == 3) {
        to_pay = action.amt - pips[active_player];
    }

    new_pips[active_player] += to_pay;
    new_stacks[active_player] -= to_pay;
    new_pot += to_pay;

    bool round_ended = false;

    if (last_action.type == 0) round_ended = true;

    else if (last_action.type != -1) {
        int a = last_action.type;
        int b = action.type;
        round_ended = (a == 1 && b == 1) || (a == 3 && b == 2);
    }

    if (round_ended ) {
        new_pips = {0, 0};
        new_active_player = 0;
        new_last_action = {-1, -1};
        new_street += 1;
    }

    if (action.type == 0) {
        new_street = 8;
    }

    return make_unique<GameState>(
        board_ref, hands_ref,
        new_stacks, new_pips,
        new_pot, new_street,
        new_active_player,
        new_last_action,
        new_packed_actions,
        new_packed_cards
    );
}

unique_ptr<GameState> GameState::sample_chance_node(mt19937& rng) const {
    PROFILE_FUNCTION();
    if (!is_chance_node()) {
        throw logic_error("sample_chance_node called when not at a chance node");
    }

    array<int, 2> new_stacks = stacks;
    array<int, 2> new_pips = {0, 0};
    int new_pot = pot;
    int new_street = street + 1;
    int new_active_player = 0;
    Action new_last_action{-1, -1};

    PackedActions new_packed_actions = packed_actions;
    array<PackedCards, 2> new_packed_cards = packed_cards;

    if (street == 0) {
        start_game(rng, board_ref, hands_ref);
        new_active_player = 1;

        new_pot += 3;
        new_stacks[0] += -2;
        new_stacks[1] += -1;
        new_pips[0] += 2;
        new_pips[1] += 1;
        
        new_packed_cards = {PackedCards{}, PackedCards{}};

        // pack hole cards (visible from street 1 onward)
        new_packed_cards[0].push(hands_ref[0]);
        new_packed_cards[0].push(hands_ref[1]);
        new_packed_cards[1].push(hands_ref[2]);
        new_packed_cards[1].push(hands_ref[3]);
    }

    // reveal public board cards on chance streets
    if (street == 2) {
        for (int i = 0; i < 3; ++i) {
            new_packed_cards[0].push(board_ref[i]);
            new_packed_cards[1].push(board_ref[i]);
        }
    }
    if (street == 4) {
        new_packed_cards[0].push(board_ref[3]);
        new_packed_cards[1].push(board_ref[3]);
    }
    if (street == 6) {
        new_packed_cards[0].push(board_ref[4]);
        new_packed_cards[1].push(board_ref[4]);
    }

    return make_unique<GameState>(
        board_ref, hands_ref, new_stacks, new_pips,
        new_pot, new_street, new_active_player,
        new_last_action,
        new_packed_actions,
        new_packed_cards
    );
}

double GameState::get_rewards(int player) const {
    PROFILE_FUNCTION();
    if (!is_terminal()) throw logic_error("Cannot assign rewards from non-terminal state");
    if (player != 0 && player != 1) throw logic_error("player must be 0 or 1");

    double win_share = 0.0;

    if (last_action.type == 0) {
        if (player == active_player) win_share = 1.0;
    } else {
        array<uint8_t, 7> player_ranks, player_suits, opp_ranks, opp_suits;

        for (int i = 0; i < 5; i++) {
            uint8_t suit = static_cast<uint8_t>(board_ref[i] % 4);
            uint8_t rank = static_cast<uint8_t>(board_ref[i] / 4); // FIX: /13 for 52 cards

            player_ranks[i] = rank;
            opp_ranks[i] = rank;

            player_suits[i] = suit;
            opp_suits[i] = suit;
        }

        for (int i = 0; i < 2; i++) {
            int pc = hands_ref[2 * player + i];
            int oc = hands_ref[2 * (1 - player) + i];

            player_suits[5 + i] = static_cast<uint8_t>(pc % 4);
            player_ranks[5 + i] = static_cast<uint8_t>(pc /4);

            opp_suits[5 + i] = static_cast<uint8_t>(oc % 4);
            opp_ranks[5 + i] = static_cast<uint8_t>(oc /4);
        }

        uint32_t player_score = evaluate_raw(player_ranks.data(), player_suits.data(), 7);
        uint32_t opp_score = evaluate_raw(opp_ranks.data(), opp_suits.data(), 7);

        if (player_score > opp_score) win_share = 1.0;
        else if (player_score == opp_score) win_share = 0.5;
    }
    return stacks[player] - starting_stack + win_share * static_cast<double>(pot);
}

InfoKey GameState::get_ID(int player) const{
    PROFILE_FUNCTION();
    return InfoKey{
        packed_cards[player].w, 
        packed_actions.w, 
        InfoKey::pack_state(pot, pips[player], pips[1 - player])
    };
}