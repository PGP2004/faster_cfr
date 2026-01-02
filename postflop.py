from typing import List, Tuple
import random
import eval7

suit_to_idx = {s: i for i, s in enumerate("cdhs")}

def make_canonical(hero: List[eval7.Card], board: List[eval7.Card]) -> Tuple[List[eval7.Card], List[eval7.Card]]:
    def key(c: eval7.Card):
        return (-c.rank, suit_to_idx[c.suit])
    return sorted(hero, key=key), sorted(board, key=key)

def batched_rollouts(hero: List[eval7.Card], opp: List[eval7.Card],board: List[eval7.Card], rem_cards: List[eval7.Card]) -> float:

    need = 5 - len(board)
    if need < 0:
        raise ValueError("board length must be <= 5")
    if len(hero) != 2 or len(opp) != 2:
        raise ValueError("hero and opp must each have 2 cards")

    if need == 0:
        hs = eval7.evaluate(hero + board)
        os = eval7.evaluate(opp + board)
        return 1.0 if hs > os else 0.5 if hs == os else 0.0

    m = len(rem_cards) // need
    if m == 0:
        raise ValueError(f"Not enough cards for a rollout: need={need}, have={len(rem_cards)}")

    total = 0.0
    base_board = list(board)

    for t in range(m):
        runout = rem_cards[t * need : (t + 1) * need]
        full_board = base_board + runout

        hs = eval7.evaluate(hero + full_board)
        os = eval7.evaluate(opp + full_board)

        if hs > os:
            total += 1.0
        elif hs == os:
            total += 0.5

    return total / m

def equity_vs_random(hero: List[eval7.Card],board: List[eval7.Card], deck_cards: List[eval7.Card],
    opp_card_samples: int = 2000) -> Tuple[float, float]:   # 10k is heavy;
    hero, board = make_canonical(hero, board)
    dead = set(hero + board)

    rem = [c for c in deck_cards if c not in dead]

    total = 0.0
    for _ in range(opp_card_samples):
        random.shuffle(rem)
        opp = rem[:2]
        rem_after_opp = rem[2:]  

        total += batched_rollouts(hero, opp, board, rem_after_opp)

    equity = total / opp_card_samples
    return (equity, equity * equity)

#TODO: need to make a function to like go through all possible canonical like hand + board pairs, then iterate and store all of the
#like tuples


def main():
    # Choose bucket counts per street (typical: more for flop than river)
    configs = [
        ("flop",  400),
        ("turn",  300),
        ("river", 200),
    ]

    NUM_SAMPLES = 25000
    EQUITY_ITERS = 800
    SEED = 1337

    all_out = {}
    for street, k in configs:
        out = build_buckets(
            street=street,
            num_samples=NUM_SAMPLES,
            equity_iters=EQUITY_ITERS,
            k=k,
            seed=SEED + hash(street) % 10000,
        )
        all_out[street] = out

    with open("postflop_bucket_centers.json", "w") as f:
        json.dump(all_out, f)

    print("\nWrote: postflop_bucket_centers.json")


if __name__ == "__main__":
    main()
