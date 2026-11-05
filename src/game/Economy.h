
#pragma once

#include "PlayerManager.h"
#include "Config.h"

#include <glm/glm.hpp>
#include <vector>

class Economy {
public:
    // X = 10 coins.
    static constexpr int MELEE_COST = 10;
    static constexpr int RANGED_COST = 2 * MELEE_COST;
    static constexpr int DEFENSE_COST = 3 * MELEE_COST;

    void reset();

    // Create a collectible when an enemy dies.
    void addDrop(int lane, float enemyX);

    // Check if a player touches a collectible.
    void update(const PlayerManager& players);

    // Purchase a player by clicking its card.
    void handleCardClick(
        const Game& game,
        PlayerManager& players
    );

    void drawCollectibles(Renderer& r) const;
    void drawHud(Renderer& r, glm::vec2 mouse) const;

    int coins() const { return coins_; }

    bool available(PlayerKind kind) const;

private:
    struct Drop {
        int lane;
        float x;
        int value;
    };

    int coins_ = 3 * MELEE_COST;
    int earnedFromDrops_ = 0;

    std::vector<Drop> drops_;
};
