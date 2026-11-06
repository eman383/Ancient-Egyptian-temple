
#pragma once

#include <vector>

class PlayerManager;
class Renderer;

constexpr int STARTING_COINS = 30;

struct Economy {
    // Shared wallet used by your teammate's CardsUI.
    int coins = STARTING_COINS;

    void reset();
    void add(int amount);
    bool canAfford(int cost) const;
    bool spend(int cost);

    // Track coins earned from defeated enemies separately.
    int earnedFromDrops() const {
        return earnedFromDrops_;
    }

    // Enemy death creates a collectible.
    void addDrop(int lane, float x);

    // Living players collect coins.
    void update(const PlayerManager& players);

    // Draw coins in their lanes.
    void drawCollectibles(Renderer& renderer) const;

private:
    struct Drop {
        int lane;
        float x;
        int value;
    };

    int earnedFromDrops_ = 0;
    std::vector<Drop> drops_;
};
