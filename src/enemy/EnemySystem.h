#pragma once

#include "EnemyTypes.h"
#include "core/Game.h"

#include <vector>
#include <cstddef>

class EnemySystem {
public:
    // Create the initial enemies
    void reset();

    // Update positions and attacks
    void update(Game& game, float dt);

    // Draw the three characters
    void drawEnemies(Renderer& r) const;

    // Draw weapons and health bars
    void drawAttacksAndHealth(Renderer& r) const;

    // Access enemies for future player collision logic
    const std::vector<Enemy>& enemies() const {
        return enemies_;
    }

    // Access projectiles for future hit detection
    const std::vector<EnemyProjectile>& projectiles() const {
        return projectiles_;
    }

    // Decrease an enemy's health
    void damageEnemy(std::size_t index, int amount);

private:
    // Create one enemy on a specific lane
    void spawn(EnemyKind type, int lane);

    // Draw the segmented health bar
    void drawHealthBar(
        Renderer& r,
        const Enemy& enemy
    ) const;

    // Temporary test function for keys 7, 8, 9
    void damageFirstOfType(EnemyKind type);

    std::vector<Enemy> enemies_;
    std::vector<EnemyProjectile> projectiles_;
};
