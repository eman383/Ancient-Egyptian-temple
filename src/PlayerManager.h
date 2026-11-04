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

// Where an incoming enemy attack travels. The enemy side picks one per attack:
//   High  - passes at head height: a DUCKING player dodges it.
//   Low   - passes along the ground: a JUMPING player dodges it.
//   Any   - cannot be dodged by anyone (close-range attacks, e.g. Medusa's eye lasers).
// Defense players never dodge, whatever the height.
enum class AttackHeight {
    Any,   // Medusa's close-range attack: cannot be dodged
    High,  // Mummy: duck to avoid
    Low    // Pharaoh: jump to avoid
};

struct PlayerHitResult {
    std::size_t playerId;
    PlayerKind kind;
    bool defeated;
    bool dodged = false;   // true: the target dodged, so no damage was dealt
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

    // ---------------- placement system ----------------
    // 1) Something (dev keys now, D's cards later) picks a type with selectType().
    // 2) placeOnSelectedLane() puts one player of that type on the selected lane.
    //    Rule: place ONLY when exactly one lane is selected, otherwise do nothing.
    void selectType(PlayerKind kind)           { selectedType_ = kind; }
    void clearSelectedType()                   { selectedType_.reset(); }
    std::optional<PlayerKind> selectedType() const { return selectedType_; }

    // Returns the new player's id, or std::nullopt when nothing was placed
    // (no type selected, zero or several lanes selected, or the placement is not allowed).
    // x < 0 (default) = left edge of the field; the dev key passes the mouse x to test far-right spots.
    std::optional<PlayerId> placeOnSelectedLane(Game& game, float x = -1.0f);

    // Call once per enemy per frame (the bridge in main.cpp does this). Any walking player in
    // that lane whose body touches the enemy's x-range [enemyXMin, enemyXMax] stops walking
    // for the next frame. Players keep attacking regardless. When nothing touches them any
    // more (enemy died or left) they walk again by themselves - nobody has to call setEngaged(false).
    void notifyEnemyContact(int lane, float enemyXMin, float enemyXMax);

    // Manual override. NOTE: update() recomputes "engaged" every frame from notifyEnemyContact().
    bool setEngaged(PlayerId id, bool engaged);
    bool isAlive(PlayerId id) const;

    // enemyX is the enemy's centre. The nearest living player at or to its left
    // is the target. A Defense player between the enemy and another player
    // naturally intercepts the attack while it remains alive.
    // If the target dodges (see AttackHeight) it takes no damage, the result has
    // dodged == true and defeated == false. Otherwise it takes the full damage.
    // Returns std::nullopt when there is no target or the arguments are invalid.
    std::optional<PlayerHitResult> applyEnemyAttack(int lane, float attackX, int damage,
                                                AttackHeight height = AttackHeight::Any,
                                                float hitRadius = 4.0f);

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
        bool touchingEnemy = false;   // set by notifyEnemyContact(), consumed by update()
    };

    // ===== TEMPORARY STAND-IN (waiting on C: coin balance, costs, unlock rules) =====
    // Always allows placement. When C's economy exists, replace the body with the real
    // check ("enough coins and unlocked") and charge the cost after a successful place().
    bool canPlace(PlayerKind kind) const;
    // ===== END STAND-IN =====

    std::optional<PlayerId> place(PlayerKind kind, int lane, float x);
    void update(Game& game, float dt);
    void drawEntities(Renderer& renderer) const;
    void drawEffects(Renderer& renderer) const;
    Entry* find(PlayerId id);
    const Entry* find(PlayerId id) const;

    std::vector<Entry> players_;
    PlayerId nextId_ = 1;
    std::optional<PlayerKind> selectedType_;   // set by dev keys now, by D's cards later
    bool setup_ = false;
};