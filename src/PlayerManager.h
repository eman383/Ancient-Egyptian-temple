
#pragma once

#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "core/Game.h"

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

struct Enemy; // Forward declaration; defined in enemy/EnemyTypes.h.

enum class PlayerKind { Melee, Defense, LongRange };

// CHANGED: classify attacks by HOW players avoid them.
enum class AttackHeight { High, Low, Melee };

struct PlayerHitResult {
    std::size_t playerId;
    PlayerKind kind;
    bool defeated;
    int remainingHealth;
    float x; // Useful for positioning the Pharaoh's lightning effect.
};

// A snapshot lets the enemy system inspect players without owning them.
struct PlayerSnapshot {
    std::size_t id;
    PlayerKind kind;
    int lane;
    float x;
    float halfWidth;
    int health;
    bool jumping;
    bool ducking;
};

class PlayerManager {
public:
    using PlayerId = std::size_t;

    // CHANGED: main.cpp uses one ordered combat update; drawing/reset still use hooks.
    void setup(Game& game, bool registerOwnUpdate = true);
    void clear();

    std::optional<PlayerId> placeMelee(int lane, float x = -1.0f);
    std::optional<PlayerId> placeDefense(int lane, float x = -1.0f);
    std::optional<PlayerId> placeLongRange(int lane, float x = -1.0f);

    // Called by main.cpp once per frame, after engagement is calculated.
    void update(float dt, const std::vector<int>& selectedLanes = {},
                bool jumpPressed = false, bool duckHeld = false);

    // CHANGED: compute contact BEFORE players move, including the current frame's approach.
    void setEngagements(const std::vector<Enemy>& enemies, float dt);
    std::vector<PlayerSnapshot> snapshots() const;

    bool setEngaged(PlayerId id, bool engaged);
    bool isAlive(PlayerId id) const;

    // CHANGED: resolve one attack from RIGHT to LEFT. Defenders intercept;
    // ducking avoids High, jumping avoids Low, neither avoids Melee.
    // Return a hit only when health was actually reduced.
    std::optional<PlayerHitResult> applyEnemyAttack(
        int lane, float fromX, float toX, int damage,
        AttackHeight height, float hitRadius = 4.0f);

    // Called once per enemy, in LEFT-TO-RIGHT order per lane, to ensure arrows
    // cannot pass through a nearer enemy to hit one farther away.
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
    void drawEntities(Renderer& renderer) const;
    void drawEffects(Renderer& renderer) const;
    Entry* find(PlayerId id);
    const Entry* find(PlayerId id) const;

    std::vector<Entry> players_;
    PlayerId nextId_ = 1;
    bool setup_ = false;
};
