#include "Utils.h"
#include "Packings.h"
#include "GameState.h"
#include <algorithm>
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
      action_history(),
      last_action({-1, -1}),
      packed_actions(),
      packed_cards() {

    // cout << "The length of actions is: " << packed_actions.len << "\n";
    // cout << "Action History: ";
    
    // for (const Action& action : action_history){
    //     cout << "( " << action.type << " , " << action.amt << packed_actions.len << ")" << "\n";
    //   }
    // cout << "\n";
    }

GameState::GameState(array<int, 5>& cur_board_ref,
                     array<int, 4>& cur_hands_ref,
                     array<int, 2> cur_stacks,
                     array<int, 2> cur_pips,
                     int cur_pot,
                     int cur_street,
                     int cur_active_player,
                     vector<Action> cur_history,
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
      action_history(std::move(cur_history)),
      last_action(cur_last_action),
      packed_actions(cur_packed_actions),
      packed_cards(cur_packed_cards) {

    // cout << "The length of packed actions is: " << packed_actions.len << "\n";
    // cout << "The length of action history is: " << action_history.size() << "\n";

    //     vector<string> action_type_name = {
    // "FOLD",   // 0
    // "CHECK",  // 1
    // "CALL",   // 2
    // "RAISE"   // 3
    // };

    // cout << "The final action history is: ";

    // for (const Action& action: action_history){
    //     cout << "(" << action_type_name[action.type] << ",";
    //     if (action.type == 3){
    //         cout << action.amt ;
    //     }
    //     cout << ")";
    // }
    // cout << "\n";
    
    // for (const Action& action : action_history){
    //     cout << "( " << action.type << " , " << action.amt << ")" << "\n";
    //   }
    // cout << "\n";
    }

bool GameState::is_legal_action(const Action& action) const {
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

bool GameState::is_terminal() const {

//     vector<string> action_type_name = {
//     "FOLD",   // 0
//     "CHECK",  // 1
//     "CALL",   // 2
//     "RAISE"   // 3
// };

//     cout << "The final action history is: ";

//     for (const Action& action: action_history){
//         cout << "(" << action_type_name[action.type] << ",";
//         if (action.type == 3){
//             cout << action.amt ;
//         }
//         cout << ")";
//     }
//     cout << "\n";

    return street == 8;
}

bool GameState::is_chance_node() const {
    return (street % 2 == 0 && street != 8);
}

int GameState::get_active_player() const {
    return active_player;
}

int GameState::get_pot() const {
    return pot;
}

int GameState::get_pip(int player) const {
    if (player != 0 && player != 1) {
        throw logic_error("The player index must be one of 1 or 0");
    }
    return pips[player];
}

const vector<Action>& GameState::get_action_history() const {
    return action_history;
}

array<int, 2> GameState::get_hand(int player) const {
    if (player != 0 && player != 1) {
        throw logic_error("The player index must be one of 1 or 0");
    }
    return {hands_ref[2 * player], hands_ref[2 * player + 1]};
}

array<int, 5> GameState::get_board() const {
    int n = 0;
    if (street < 2) n = 0;
    else if (street < 4) n = 3;
    else if (street < 6) n = 4;
    else n = 5;

    array<int, 5> output;
    output.fill(-1);
    for (int i = 0; i < n; i++) output[i] = board_ref[i];
    return output;
}

unique_ptr<GameState> GameState::next_game_state(const Action& action) const {
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

    vector<Action> new_history = action_history;
    new_history.push_back(action);
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
        std::move(new_history),
        new_last_action,
        new_packed_actions,
        new_packed_cards
    );
}

unique_ptr<GameState> GameState::sample_chance_node(mt19937& rng) const {
    if (!is_chance_node()) {
        throw logic_error("sample_chance_node called when not at a chance node");
    }

    array<int, 2> new_stacks = stacks;
    array<int, 2> new_pips = {0, 0};
    int new_pot = pot;
    int new_street = street + 1;
    int new_active_player = 0;

    vector<Action> new_history = action_history;
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
        std::move(new_history),
        new_last_action,
        new_packed_actions,
        new_packed_cards
    );
}

double GameState::get_rewards(int player) const {
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

pair<uint64_t, uint64_t> GameState::get_ID(int player) const{
    pair<uint64_t, uint64_t> output = {packed_cards[player].w, packed_actions.w};
    return output;
}