#include "PlayerManager.h"

#include "Config.h"

#include <algorithm>
#include <type_traits>

namespace {
constexpr float PLAYER_HEIGHT = LANE_H * 0.80f;
}

void PlayerManager::setup(Game& game)
{
    if (setup_) return;
    setup_ = true;

    game.onGameplayUpdate([this, &game](float dt) { update(game, dt); });
    game.onDraw(Layer::Entities, [this](Renderer& renderer) { drawEntities(renderer); });
    game.onDraw(Layer::Effects, [this](Renderer& renderer) { drawEffects(renderer); });
    game.onReset([this] { clear(); });
    clear();
}

void PlayerManager::clear()
{
    players_.clear();
    nextId_ = 1;
    selectedType_.reset();
}

// ===== TEMPORARY STAND-IN (waiting on C) =====
bool PlayerManager::canPlace(PlayerKind) const
{
    return true;
}
// ===== END STAND-IN =====

std::optional<PlayerManager::PlayerId> PlayerManager::placeOnSelectedLane(Game& game, float x)
{
    if (!selectedType_) return std::nullopt;

    // Exactly one selected lane, otherwise do nothing.
    const std::vector<int> lanes = game.lanes.selectedLanes();
    if (lanes.size() != 1) return std::nullopt;

    if (!canPlace(*selectedType_)) return std::nullopt;

    return place(*selectedType_, lanes.front(), x);
}

std::optional<PlayerManager::PlayerId> PlayerManager::placeMelee(int lane, float x)
{
    return place(PlayerKind::Melee, lane, x);
}

std::optional<PlayerManager::PlayerId> PlayerManager::placeDefense(int lane, float x)
{
    return place(PlayerKind::Defense, lane, x);
}

std::optional<PlayerManager::PlayerId> PlayerManager::placeLongRange(int lane, float x)
{
    return place(PlayerKind::LongRange, lane, x);
}

std::optional<PlayerManager::PlayerId> PlayerManager::place(PlayerKind kind, int lane, float x)
{
    if (lane < 0 || lane >= LANE_COUNT) return std::nullopt;
    if (x < 0.0f) x = FIELD_X0;
    x = std::max(FIELD_X0, x);

    const glm::vec2 feet(x, laneBottomY(lane));
    PlayerId id = nextId_++;
    switch (kind) {
        case PlayerKind::Melee:
            players_.push_back({id, lane, kind, MeleePlayer(feet, PLAYER_HEIGHT)});
            break;
        case PlayerKind::Defense:
            players_.push_back({id, lane, kind, DefensePlayer(feet, PLAYER_HEIGHT)});
            break;
        case PlayerKind::LongRange:
            players_.push_back({id, lane, kind, LongRangePlayer(feet, PLAYER_HEIGHT)});
            // Cull arrows once they are well off-screen. Without this they fly forever,
            // pile up, and a defeated archer can never be removed (isRemovable() waits
            // for her arrows to be gone).
            std::get<LongRangePlayer>(players_.back().actor).setArrowMaxX(VIRTUAL_W + 200.0f);
            break;
    }
    return id;
}

void PlayerManager::update(Game& game, float dt)
{
    const std::vector<int> selectedLanes = game.lanes.selectedLanes();
    const auto laneSelected = [&selectedLanes](int lane) {
        return std::find(selectedLanes.begin(), selectedLanes.end(), lane) != selectedLanes.end();
    };

    if (game.input.jumpPressed() || game.input.duckPressed()) {
        for (Entry& entry : players_) {
            if (!laneSelected(entry.lane) || entry.kind == PlayerKind::Defense) continue;
            std::visit([&](auto& actor) {
                using T = std::decay_t<decltype(actor)>;
                if constexpr (!std::is_same_v<T, DefensePlayer>) {
                    if (game.input.jumpPressed()) actor.jump();
                    if (game.input.duckPressed()) actor.duck();
                }
            }, entry.actor);
        }
    }

    // Stop walking only while touching an enemy; walk again the frame after contact ends.
    for (Entry& entry : players_) {
        if (entry.kind == PlayerKind::Defense) continue;
        setEngaged(entry.id, entry.touchingEnemy);
        entry.touchingEnemy = false;
    }

    for (Entry& entry : players_)
        std::visit([dt](auto& actor) { actor.update(dt); }, entry.actor);

    // Removed when defeated, or when a walker reaches the right edge of the field
    // (EDGE RULE: a player that walks off the right side is removed, never stuck there).
    players_.erase(std::remove_if(players_.begin(), players_.end(), [](const Entry& entry) {
        if (entry.kind != PlayerKind::Defense) {
            const float x = std::visit([](const auto& actor) { return actor.position().x; }, entry.actor);
            if (x >= FIELD_X1) return true;
        }
        return std::visit([](const auto& actor) {
            using T = std::decay_t<decltype(actor)>;
            if constexpr (std::is_same_v<T, LongRangePlayer>) return actor.isRemovable();
            else return !actor.isAlive();
        }, entry.actor);
    }), players_.end());
}

void PlayerManager::drawEntities(Renderer& renderer) const
{
    for (const Entry& entry : players_)
        std::visit([&renderer](const auto& actor) { actor.draw(renderer); }, entry.actor);
}

void PlayerManager::drawEffects(Renderer& renderer) const
{
    for (const Entry& entry : players_)
        std::visit([&renderer](const auto& actor) { actor.drawEffects(renderer); }, entry.actor);
}

PlayerManager::Entry* PlayerManager::find(PlayerId id)
{
    const auto it = std::find_if(players_.begin(), players_.end(),
                                 [id](const Entry& entry) { return entry.id == id; });
    return it == players_.end() ? nullptr : &*it;
}

const PlayerManager::Entry* PlayerManager::find(PlayerId id) const
{
    const auto it = std::find_if(players_.begin(), players_.end(),
                                 [id](const Entry& entry) { return entry.id == id; });
    return it == players_.end() ? nullptr : &*it;
}

void PlayerManager::notifyEnemyContact(int lane, float enemyXMin, float enemyXMax)
{
    if (lane < 0 || lane >= LANE_COUNT || enemyXMin > enemyXMax) return;
    for (Entry& entry : players_) {
        if (entry.lane != lane || entry.kind == PlayerKind::Defense) continue;
        const bool touching = std::visit([&](const auto& actor) {
            using T = std::decay_t<decltype(actor)>;
            if constexpr (std::is_same_v<T, DefensePlayer>) {
                return false;
            } else {
                if (!actor.isAlive()) return false;
                const float x = actor.position().x;
                // Front of the body reaches the enemy, and he has not walked past it.
                return x + actor.halfWidth() >= enemyXMin && x <= enemyXMax;
            }
        }, entry.actor);
        if (touching) entry.touchingEnemy = true;
    }
}

bool PlayerManager::setEngaged(PlayerId id, bool engaged)
{
    Entry* entry = find(id);
    if (!entry || entry->kind == PlayerKind::Defense) return false;
    std::visit([engaged](auto& actor) {
        using T = std::decay_t<decltype(actor)>;
        if constexpr (!std::is_same_v<T, DefensePlayer>) actor.setEngaged(engaged);
    }, entry->actor);
    return true;
}

bool PlayerManager::isAlive(PlayerId id) const
{
    const Entry* entry = find(id);
    return entry && std::visit([](const auto& actor) { return actor.isAlive(); }, entry->actor);
}

std::optional<PlayerHitResult> PlayerManager::applyEnemyAttack(int lane, float attackX, int damage,
                                                                AttackHeight height, float hitRadius)
{
    if (damage <= 0 || lane < 0 || lane >= LANE_COUNT) return std::nullopt;

    Entry* target = nullptr;
    float targetX = -1.0f;
    for (Entry& entry : players_) {
        if (entry.lane != lane) continue;
        const bool alive = std::visit([](const auto& actor) { return actor.isAlive(); }, entry.actor);
        if (!alive) continue;
        const float x = std::visit([](const auto& actor) { return actor.position().x; }, entry.actor);
        if (x > attackX + hitRadius) continue;
        const bool defenseWinsTie = target && x == targetX && entry.kind == PlayerKind::Defense &&
                                    target->kind != PlayerKind::Defense;
        if (!target || x > targetX || defenseWinsTie) {
            target = &entry;
            targetX = x;
        }
    }
    if (!target) return std::nullopt;

    // Jump dodges low attacks, duck dodges high attacks. Melee is never dodged and
    // a Defense player never dodges (it ignores jump/duck).
    const bool dodged = std::visit([height](const auto& actor) {
        using T = std::decay_t<decltype(actor)>;
        if constexpr (std::is_same_v<T, DefensePlayer>) {
            return false;
        } else {
            return (height == AttackHeight::Low  && actor.isJumping()) ||
                   (height == AttackHeight::High && actor.isDucking());
        }
    }, target->actor);
    if (dodged) return PlayerHitResult{target->id, target->kind, false, true};

    std::visit([damage](auto& actor) { actor.takeHit(damage); }, target->actor);
    return PlayerHitResult{target->id, target->kind,
        !std::visit([](const auto& actor) { return actor.isAlive(); }, target->actor)};
}

int PlayerManager::applyPlayerAttacksToEnemy(int lane, float enemyXMin, float enemyXMax)
{
    if (lane < 0 || lane >= LANE_COUNT || enemyXMin > enemyXMax) return 0;

    int damage = 0;
    for (Entry& entry : players_) {
        if (entry.lane != lane) continue;
        std::visit([&](auto& actor) {
            using T = std::decay_t<decltype(actor)>;
            if constexpr (std::is_same_v<T, MeleePlayer>) {
                if (!actor.isAlive()) return;
                if (actor.attackHits(enemyXMin, enemyXMax)) damage += MeleePlayer::DAMAGE;
            } else if constexpr (std::is_same_v<T, LongRangePlayer>) {
                // No isAlive() check: arrows already in flight keep flying (and are still
                // drawn) after the archer is defeated, so they must still be able to hit.
                for (std::size_t i = 0; i < actor.arrows().size(); ++i)
                    if (actor.arrowHitsEnemy(i, enemyXMin, enemyXMax)) damage += LongRangePlayer::DAMAGE;
            }
        }, entry.actor);
    }
    return damage;
}