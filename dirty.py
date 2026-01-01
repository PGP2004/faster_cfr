if __name__ == "__main__":
    pot = 3                  # starting pot
    chips_remaining = 397    # effective stack
    n = 0


    while True:
        bet = pot // 2
        if bet <= 0 or bet > chips_remaining:
            break

        chips_remaining -= bet
        pot += bet
        n += 1


    print("\nTotal half-pot raises:", n)