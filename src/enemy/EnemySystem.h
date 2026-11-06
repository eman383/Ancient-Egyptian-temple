
#pragma once

#include "EnemyTypes.h"
#include "core/Game.h"

#include <cstddef>
#include <functional>
#include <random>
#include <utility>
#include <vector>

class PlayerManager;

class EnemySystem {
public:
    void reset();
    void update(Game& game, PlayerManager& players, float dt);

    void drawEnemies(Renderer& r) const;
    void drawAttacksAndHealth(Renderer& r) const;

    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<EnemyProjectile>& projectiles() const { return projectiles_; }

    void damageEnemy(std::size_t index, int amount);
    int wave() const { return wave_; }

    // Preserve the economy callback: one collectible for each defeated enemy.
    void onEnemyDefeated(std::function<void(int, float)> fn) {
        onDefeated_ = std::move(fn);
    }

#ifdef TEMPLE_COMBAT_TESTS
    // Only compiled into the automated test executable, never the game.
    void spawnForTest(EnemyKind kind, int lane, float x);
#endif

private:
    void spawn(EnemyKind type, int lane, float x = FIELD_X1);
    bool spawnNext(); // Existing 3-wave random-lane logic.
    void drawHealthBar(Renderer& r, const Enemy& enemy) const;

    std::vector<Enemy> enemies_;
    std::vector<EnemyProjectile> projectiles_;

    int wave_ = 1;
    int spawned_ = 0;
    float spawnClock_ = 0.5f;
    std::mt19937 rng_{std::random_device{}()};
    std::function<void(int, float)> onDefeated_;

#ifdef TEMPLE_COMBAT_TESTS
    bool manualTestMode_ = false; // Keep test matchups independent of random waves.
#endif
};
