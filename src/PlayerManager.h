
#pragma once

#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "core/Game.h"

#include <cstddef>
#include <optional>
#include <variant>
#include <vector>

struct Enemy;

enum class PlayerKind {
    Melee,
    Defense,
    LongRange
};

enum class AttackHeight {
    High,
    Low,
    Melee
};

struct PlayerHitResult {
    std::size_t playerId;
    PlayerKind kind;
    bool defeated;
    int remainingHealth;
    float x;
};

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

    void setup(Game& game, bool registerOwnUpdate = true);
    void clear();

    // Player type chosen on a card.
    void selectType(PlayerKind kind) {
        selectedType_ = kind;
    }

    void clearSelectedType() {
        selectedType_.reset();
    }

    std::optional<PlayerKind> selectedType() const {
        return selectedType_;
    }

    // Requires exactly one selected lane.
    std::optional<PlayerId> placeOnSelectedLane(
        Game& game, float x = -1.0f
    );

    std::optional<PlayerId> placeMelee(
        int lane, float x = -1.0f
    );

    std::optional<PlayerId> placeDefense(
        int lane, float x = -1.0f
    );

    std::optional<PlayerId> placeLongRange(
        int lane, float x = -1.0f
    );

    // Combined player movement and dodging.
    void update(
        float dt,
        const std::vector<int>& selectedLanes = {},
        bool jumpPressed = false,
        bool duckHeld = false
    );

    // Stop players when enemies are approaching.
    void setEngagements(
        const std::vector<Enemy>& enemies,
        float dt
    );

    std::vector<PlayerSnapshot> snapshots() const;

    bool setEngaged(PlayerId id, bool engaged);
    bool isAlive(PlayerId id) const;

    // Locked-shield rule (TA): a Defense player can only be placed while at least one
    // attacking player (Melee or Long Range) is alive somewhere on the field.
    // The shield itself may then go on ANY lane.
    bool hasLivingAttacker() const;

    std::optional<PlayerHitResult> applyEnemyAttack(
        int lane,
        float fromX,
        float toX,
        int damage,
        AttackHeight height,
        float hitRadius = 4.0f
    );

    int applyPlayerAttacksToEnemy(
        int lane,
        float enemyXMin,
        float enemyXMax
    );

private:
    using Actor = std::variant<
        DefensePlayer,
        LongRangePlayer,
        MeleePlayer
    >;

    struct Entry {
        PlayerId id;
        int lane;
        PlayerKind kind;
        Actor actor;
    };

    std::optional<PlayerId> place(
        PlayerKind kind,
        int lane,
        float x
    );

    void drawEntities(Renderer& renderer) const;
    void drawEffects(Renderer& renderer) const;

    Entry* find(PlayerId id);
    const Entry* find(PlayerId id) const;

    std::vector<Entry> players_;
    PlayerId nextId_ = 1;
    std::optional<PlayerKind> selectedType_;
    bool setup_ = false;
};
