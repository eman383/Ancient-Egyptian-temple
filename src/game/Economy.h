#pragma once

// The player's coin balance (points / collectibles). Shared by the cards UI (D), the coin
// pickups (C) and anything else that needs it.
//   - C: when a collectible is picked up, call economy.add(value).
//   - The cards UI spends coins itself when a card places a player.
// The game starts with STARTING_COINS (enough for 3 of the cheapest player).
constexpr int STARTING_COINS = 30;

struct Economy {
    int coins = STARTING_COINS;

    void add(int amount)       { coins += amount; }
    bool canAfford(int cost) const { return coins >= cost; }
    bool spend(int cost) {
        if (cost > coins) return false;
        coins -= cost;
        return true;
    }
};
