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

    for (Entry& entry : players_)
        std::visit([dt](auto& actor) { actor.update(dt); }, entry.actor);

    players_.erase(std::remove_if(players_.begin(), players_.end(), [](const Entry& entry) {
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

std::optional<PlayerHitResult> PlayerManager::applyEnemyAttack(int lane, float enemyX, int damage)
{
    if (damage <= 0 || lane < 0 || lane >= LANE_COUNT) return std::nullopt;

    Entry* target = nullptr;
    float targetX = -1.0f;
    for (Entry& entry : players_) {
        if (entry.lane != lane) continue;
        const bool alive = std::visit([](const auto& actor) { return actor.isAlive(); }, entry.actor);
        if (!alive) continue;
        const float x = std::visit([](const auto& actor) { return actor.position().x; }, entry.actor);
        if (x > enemyX) continue;
        const bool defenseWinsTie = target && x == targetX && entry.kind == PlayerKind::Defense &&
                                    target->kind != PlayerKind::Defense;
        if (!target || x > targetX || defenseWinsTie) {
            target = &entry;
            targetX = x;
        }
    }
    if (!target) return std::nullopt;

    std::visit([damage](auto& actor) { actor.takeHit(damage); }, target->actor);
    if (target->kind != PlayerKind::Defense) setEngaged(target->id, true);
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
            if (!actor.isAlive()) return;
            if constexpr (std::is_same_v<T, MeleePlayer>) {
                if (actor.attackHits(enemyXMin, enemyXMax)) damage += MeleePlayer::DAMAGE;
            } else if constexpr (std::is_same_v<T, LongRangePlayer>) {
                for (std::size_t i = 0; i < actor.arrows().size(); ++i)
                    if (actor.arrowHitsEnemy(i, enemyXMin, enemyXMax)) damage += LongRangePlayer::DAMAGE;
            }
        }, entry.actor);
    }
    return damage;
}
