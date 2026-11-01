#pragma once

#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "core/Game.h"

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

enum class PlayerKind { Melee, Defense, LongRange };

struct PlayerHitResult {
    std::size_t playerId;
    PlayerKind kind;
    bool defeated;
};

// Owns player instances and connects them to the shared Game update/draw hooks.
// The enemy system calls applyEnemyAttack() for incoming attacks and
// applyPlayerAttacksToEnemy() for enemy-only player attack collision checks.
class PlayerManager {
public:
    using PlayerId = std::size_t;

    void setup(Game& game);  // call once before Game::init()/Game::run()
    void clear();

    std::optional<PlayerId> placeMelee(int lane, float x = -1.0f);
    std::optional<PlayerId> placeDefense(int lane, float x = -1.0f);
    std::optional<PlayerId> placeLongRange(int lane, float x = -1.0f);

    bool setEngaged(PlayerId id, bool engaged);
    bool isAlive(PlayerId id) const;

    // enemyX is the enemy's centre. The nearest living player at or to its left
    // receives the complete attack damage. A Defense player between the enemy
    // and another player naturally intercepts the attack while it remains alive.
    std::optional<PlayerHitResult> applyEnemyAttack(int lane, float enemyX, int damage);

    // Enemy-only collision entry point. Call once per enemy per frame with its
    // horizontal body interval. Arrows and melee strikes are never tested against players.
    int applyPlayerAttacksToEnemy(int lane, float enemyXMin, float enemyXMax);

private:
    using Actor = std::variant<DefensePlayer, LongRangePlayer, MeleePlayer>;
    struct Entry {
        PlayerId id;
        int lane;
        PlayerKind kind;
        Actor actor;
    };

    std::optional<PlayerId> place(PlayerKind kind, int lane, float x);
    void update(Game& game, float dt);
    void drawEntities(Renderer& renderer) const;
    void drawEffects(Renderer& renderer) const;
    Entry* find(PlayerId id);
    const Entry* find(PlayerId id) const;

    std::vector<Entry> players_;
    PlayerId nextId_ = 1;
    bool setup_ = false;
};
