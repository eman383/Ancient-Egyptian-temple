
#include "PlayerManager.h"

#include "Config.h"
#include "enemy/EnemyTypes.h"
#include "audio/Audio.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

namespace {

constexpr float PLAYER_HEIGHT = LANE_H * 0.80f;

} // namespace

void PlayerManager::setup(
    Game& game,
    bool registerOwnUpdate
) {
    if (setup_) return;

    setup_ = true;

    // main.cpp supplies false so gameplay runs in one
    // ordered callback, rather than updating players twice.
    if (registerOwnUpdate) {
        game.onGameplayUpdate([this, &game](float dt) {
            update(
                dt,
                game.lanes.selectedLanes(),
                game.input.jumpPressed(),
                game.input.key(GLFW_KEY_S) ||
                    game.input.key(GLFW_KEY_DOWN)
            );
        });
    }

    // Keep the original player drawing hooks.
    game.onDraw(Layer::Entities, [this](Renderer& r) {
        drawEntities(r);
    });

    game.onDraw(Layer::Effects, [this](Renderer& r) {
        drawEffects(r);
    });

    game.onReset([this] {
        clear();
    });

    clear();
}

void PlayerManager::clear() {
    players_.clear();
    nextId_ = 1;

    // Reset the currently selected player card.
    selectedType_.reset();
}

// Exactly one lane must be selected for player placement.
std::optional<PlayerManager::PlayerId>
PlayerManager::placeOnSelectedLane(Game& game, float x) {

    if (!selectedType_) {
        return std::nullopt;
    }

    const auto lanes = game.lanes.selectedLanes();

    if (lanes.size() != 1) {
        return std::nullopt;
    }

    return place(*selectedType_, lanes.front(), x);
}

std::optional<PlayerManager::PlayerId>
PlayerManager::placeMelee(int lane, float x) {
    return place(PlayerKind::Melee, lane, x);
}

std::optional<PlayerManager::PlayerId>
PlayerManager::placeDefense(int lane, float x) {
    return place(PlayerKind::Defense, lane, x);
}

std::optional<PlayerManager::PlayerId>
PlayerManager::placeLongRange(int lane, float x) {
    return place(PlayerKind::LongRange, lane, x);
}

std::optional<PlayerManager::PlayerId>
PlayerManager::place(PlayerKind kind, int lane, float x) {

    if (lane < 0 || lane >= LANE_COUNT) {
        return std::nullopt;
    }

    // Locked shield: no Defense until an attacker is alive on the field.
    if (kind == PlayerKind::Defense && !hasLivingAttacker()) {
        return std::nullopt;
    }

    if (x < 0.0f) {
        x = FIELD_X0;
    }

    x = std::max(FIELD_X0, x);

    const glm::vec2 feet(x, laneBottomY(lane));
    const PlayerId id = nextId_++;

    switch (kind) {
        case PlayerKind::Melee:
            players_.push_back({
                id,
                lane,
                kind,
                MeleePlayer(feet, PLAYER_HEIGHT)
            });
            break;

        case PlayerKind::Defense:
            players_.push_back({
                id,
                lane,
                kind,
                DefensePlayer(feet, PLAYER_HEIGHT)
            });
            break;

        case PlayerKind::LongRange:
            players_.push_back({
                id,
                lane,
                kind,
                LongRangePlayer(feet, PLAYER_HEIGHT)
            });

            // Remove arrows after they leave the field.
            std::get<LongRangePlayer>(
                players_.back().actor
            ).setArrowMaxX(FIELD_X1);
            break;
    }

    return id;
}

// Update player movement, jumping, ducking, and death.
void PlayerManager::update(
    float dt,
    const std::vector<int>& selectedLanes,
    bool jumpPressed,
    bool duckHeld
) {
    const auto selected = [&](int lane) {
        return std::find(
            selectedLanes.begin(),
            selectedLanes.end(),
            lane
        ) != selectedLanes.end();
    };

    for (Entry& entry : players_) {

        // Defense does not jump or duck.
        if (selected(entry.lane) &&
            entry.kind != PlayerKind::Defense) {

            std::visit([&](auto& actor) {
                using T = std::decay_t<decltype(actor)>;

                if constexpr (!std::is_same_v<T, DefensePlayer>) {
                    if (jumpPressed) {
                        actor.jump();
                    }

                    // Holding S/Down renews ducking when permitted.
                    if (duckHeld) {
                        actor.duck();
                    }
                }
            }, entry.actor);
        }

        // Existing model classes still control walking,
        // attack cycles, and animation.
        std::visit(
            [dt](auto& actor) {
                actor.update(dt);
            },
            entry.actor
        );
    }

    // Keep archers until their in-flight arrows are resolved.
    players_.erase(
        std::remove_if(
            players_.begin(),
            players_.end(),
            [](const Entry& entry) {

                // Players leave the field at the right edge.
                if (entry.kind != PlayerKind::Defense) {
                    const float x = std::visit(
                        [](const auto& actor) {
                            return actor.position().x;
                        },
                        entry.actor
                    );

                    if (x >= FIELD_X1) return true;
                }

                return std::visit(
                    [](const auto& actor) {
                        using T = std::decay_t<decltype(actor)>;

                        if constexpr (
                            std::is_same_v<T, LongRangePlayer>
                        ) {
                            return actor.isRemovable();
                        } else {
                            return !actor.isAlive();
                        }
                    },
                    entry.actor
                );
            }
        ),
        players_.end()
    );
}

void PlayerManager::drawEntities(Renderer& renderer) const {
    for (const Entry& entry : players_) {
        std::visit(
            [&](const auto& actor) {
                actor.draw(renderer);
            },
            entry.actor
        );
    }
}

void PlayerManager::drawEffects(Renderer& renderer) const {
    for (const Entry& entry : players_) {
        std::visit(
            [&](const auto& actor) {
                actor.drawEffects(renderer);
            },
            entry.actor
        );
    }
}

PlayerManager::Entry* PlayerManager::find(PlayerId id) {
    auto it = std::find_if(
        players_.begin(),
        players_.end(),
        [id](const Entry& e) {
            return e.id == id;
        }
    );

    return it == players_.end() ? nullptr : &*it;
}

const PlayerManager::Entry*
PlayerManager::find(PlayerId id) const {
    auto it = std::find_if(
        players_.begin(),
        players_.end(),
        [id](const Entry& e) {
            return e.id == id;
        }
    );

    return it == players_.end() ? nullptr : &*it;
}

bool PlayerManager::setEngaged(PlayerId id, bool engaged) {
    Entry* entry = find(id);

    if (!entry || entry->kind == PlayerKind::Defense) {
        return false;
    }

    std::visit(
        [engaged](auto& actor) {
            using T = std::decay_t<decltype(actor)>;

            if constexpr (!std::is_same_v<T, DefensePlayer>) {
                actor.setEngaged(engaged);
            }
        },
        entry->actor
    );

    return true;
}

bool PlayerManager::hasLivingAttacker() const {
    for (const Entry& entry : players_) {
        if (entry.kind == PlayerKind::Defense) continue;
        const bool alive = std::visit(
            [](const auto& actor) { return actor.isAlive(); },
            entry.actor
        );
        if (alive) return true;
    }
    return false;
}

bool PlayerManager::isAlive(PlayerId id) const {
    const Entry* entry = find(id);

    return entry && std::visit(
        [](const auto& actor) {
            return actor.isAlive();
        },
        entry->actor
    );
}

// Creates a small read-only view of all living players.
std::vector<PlayerSnapshot> PlayerManager::snapshots() const {
    std::vector<PlayerSnapshot> result;
    result.reserve(players_.size());

    for (const Entry& entry : players_) {
        std::visit([&](const auto& actor) {

            if (!actor.isAlive()) return;

            using T = std::decay_t<decltype(actor)>;

            if constexpr (std::is_same_v<T, DefensePlayer>) {
                result.push_back({
                    entry.id,
                    entry.kind,
                    entry.lane,
                    actor.position().x,
                    actor.height() * 0.23f,
                    actor.health(),
                    false,
                    false
                });
            } else {
                result.push_back({
                    entry.id,
                    entry.kind,
                    entry.lane,
                    actor.position().x,
                    actor.halfWidth(),
                    actor.health(),
                    actor.isJumping(),
                    actor.isDucking()
                });
            }
        }, entry.actor);
    }

    return result;
}

// Stop a player BEFORE the next movement update if
// an enemy is currently touching or about to touch it.
void PlayerManager::setEngagements(
    const std::vector<Enemy>& enemies,
    float dt
) {
    const auto players = snapshots();

    for (const PlayerSnapshot& p : players) {

        if (p.kind == PlayerKind::Defense) continue;

        bool touching = false;

        for (const Enemy& e : enemies) {

            if (e.health <= 0 || e.lane != p.lane) continue;

            const float contact =
                e.halfWidth() + p.halfWidth;

            const float gap = e.x - p.x;

            const float closing =
                (e.speed + 0.40f * PLAYER_HEIGHT) *
                std::max(0.0f, dt);

            if (std::abs(gap) <= contact ||
                (gap > 0.0f && gap <= contact + closing)) {
                touching = true;
                break;
            }
        }

        setEngaged(p.id, touching);
    }
}

// Apply a high, low, or melee enemy attack.
std::optional<PlayerHitResult>
PlayerManager::applyEnemyAttack(
    int lane,
    float fromX,
    float toX,
    int damage,
    AttackHeight height,
    float hitRadius
) {
    if (lane < 0 || lane >= LANE_COUNT ||
        damage <= 0 || fromX < toX) {
        return std::nullopt;
    }

    auto targets = snapshots();

    // Enemy attacks travel right-to-left.
    // Resolve the player closest to the enemy first.
    std::sort(
        targets.begin(),
        targets.end(),
        [](const PlayerSnapshot& a, const PlayerSnapshot& b) {
            if (a.x != b.x) return a.x > b.x;

            // Defense wins when two characters share X.
            return a.kind == PlayerKind::Defense &&
                   b.kind != PlayerKind::Defense;
        }
    );

    for (const PlayerSnapshot& target : targets) {

        if (target.lane != lane) continue;

        // The attack must actually cross the player's body.
        if (target.x - target.halfWidth >
                fromX + hitRadius ||
            target.x + target.halfWidth <
                toX - hitRadius) {
            continue;
        }

        // Dodged projectiles pass without applying damage.
        if (target.kind != PlayerKind::Defense &&
            (
                (height == AttackHeight::High &&
                 target.ducking) ||
                (height == AttackHeight::Low &&
                 target.jumping)
            )) {
            continue;
        }

        Entry* entry = find(target.id);
        if (!entry) continue;

        // Apply damage using the original player's takeHit().
        std::visit(
            [damage](auto& actor) {
                actor.takeHit(damage);
            },
            entry->actor
        );

        const int hp = std::visit(
            [](const auto& actor) {
                return actor.health();
            },
            entry->actor
        );

        audio::play(hp == 0 ? audio::Sfx::PlayerDeath : audio::Sfx::PlayerHurt);

        return PlayerHitResult{
            target.id,
            target.kind,
            hp == 0,
            hp,
            target.x
        };
    }

    return std::nullopt;
}

// Connect the players' existing attacks to enemy health.
int PlayerManager::applyPlayerAttacksToEnemy(
    int lane,
    float enemyXMin,
    float enemyXMax
) {
    if (lane < 0 || lane >= LANE_COUNT ||
        enemyXMin > enemyXMax) {
        return 0;
    }

    int damage = 0;

    for (Entry& entry : players_) {

        if (entry.lane != lane) continue;

        std::visit([&](auto& actor) {
            using T = std::decay_t<decltype(actor)>;

            if constexpr (std::is_same_v<T, MeleePlayer>) {

                if (actor.isAlive() &&
                    actor.attackHits(enemyXMin, enemyXMax)) {
                    damage += MeleePlayer::DAMAGE;
                }

            } else if constexpr (
                std::is_same_v<T, LongRangePlayer>
            ) {

                // Arrows already in flight remain effective.
                for (std::size_t i = 0;
                     i < actor.arrows().size();
                     ++i) {

                    if (actor.arrowHitsEnemy(
                            i,
                            enemyXMin,
                            enemyXMax
                        )) {
                        damage += LongRangePlayer::DAMAGE;
                    }
                }
            }
        }, entry.actor);
    }

    return damage;
}
