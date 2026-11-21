
#include "EnemySystem.h"

#include "PlayerManager.h"
#include "Medusa.h"
#include "Mummy.h"
#include "Pharaoh.h"
#include "audio/Audio.h"

#include <algorithm>
#include <cmath>
#include <numeric>

namespace {
constexpr float NORMAL_SPEED = 38.0f;
constexpr float HARD_SPEED = 72.0f;
constexpr float WINDUP_SECONDS = 0.30f;
constexpr float MELEE_REACH = 90.0f;
constexpr float LASER_REACH = 450.0f;

int waveTarget(int wave) { return wave == 1 ? 3 : wave == 2 ? 5 : 10; }

float attackInterval(EnemyKind kind) {
    if (kind == EnemyKind::Medusa) return 1.45f;
    if (kind == EnemyKind::Mummy) return 1.85f;
    return 1.10f; // Pharaoh attacks faster and does 2 damage.
}

} // namespace

void EnemySystem::spawn(EnemyKind kind, int lane, float x) {
    if (lane < 0 || lane >= LANE_COUNT) return;

    // Pharaoh is the hard (7 HP, 2 damage, fast) melee enemy.
    const bool hard = kind == EnemyKind::Pharaoh;
    Enemy enemy{kind, lane, x,
                hard ? 7 : 5, hard ? 7 : 5,
                hard ? 2 : 1,
                hard ? HARD_SPEED : NORMAL_SPEED};

    // Stagger the first attacks while keeping all enemies automatic.
    enemy.attackTimer = kind == EnemyKind::Medusa ? 0.35f
                      : kind == EnemyKind::Mummy ? 0.55f : 0.65f;
    enemies_.push_back(enemy);
}

#ifdef TEMPLE_COMBAT_TESTS
void EnemySystem::spawnForTest(EnemyKind kind, int lane, float x) {
    manualTestMode_ = true; // Suppress random wave enemies during an isolated test.
    spawn(kind, lane, x);
}
#endif

void EnemySystem::reset() {
    enemies_.clear();
    projectiles_.clear();
    wave_ = 1;
    spawned_ = 0;
    spawnClock_ = 0.5f;
#ifdef TEMPLE_COMBAT_TESTS
    manualTestMode_ = false;
#endif
    // The onDefeated_ callback is deliberately preserved across restarts.
}

bool EnemySystem::spawnNext() {
    std::vector<int> available;
    for (int lane = 0; lane < LANE_COUNT; ++lane) {
        const bool blocked = std::any_of(enemies_.begin(), enemies_.end(), [&](const Enemy& e) {
            return e.health > 0 && e.lane == lane && e.x > FIELD_X1 - 150.0f;
        });
        if (!blocked) available.push_back(lane);
    }
    if (available.empty()) return false;

    std::uniform_int_distribution<int> pickLane(0, static_cast<int>(available.size()) - 1);
    const int lane = available[pickLane(rng_)];
    EnemyKind kind = EnemyKind::Medusa; // Wave 1: easy only.

    if (wave_ == 2) {
        // wave 2 includes BOTH easy Medusa and medium Mummy.
        const int choice = spawned_ < 2 ? spawned_
            : std::uniform_int_distribution<int>(0, 1)(rng_);
        kind = choice == 0 ? EnemyKind::Medusa : EnemyKind::Mummy;
    } else if (wave_ == 3) {
        // The first three spawns guarantee that all three types appear.
        const int choice = spawned_ < 3 ? spawned_
            : std::uniform_int_distribution<int>(0, 2)(rng_);
        kind = choice == 0 ? EnemyKind::Medusa
             : choice == 1 ? EnemyKind::Mummy : EnemyKind::Pharaoh;
    }

    spawn(kind, lane);
    ++spawned_;
    return true;
}

void EnemySystem::damageEnemy(std::size_t index, int amount) {
    if (index >= enemies_.size() || amount <= 0) return;
    Enemy& e = enemies_[index];
    if (e.health <= 0) return;
    e.health = std::max(0, e.health - amount);

    audio::play(e.health == 0 ? audio::Sfx::EnemyDeath : audio::Sfx::EnemyHit);
    if (e.health == 0 && onDefeated_) onDefeated_(e.lane, e.x);
}

void EnemySystem::update(Game& game, PlayerManager& players, float dt) {
    if (dt <= 0.0f || !game.isPlaying()) return;

    // main.cpp has already updated players in the same ordered game update.
    const auto views = players.snapshots();

    // ========== ENEMY MOVEMENT AND ATTACK TIMERS ==========
    for (Enemy& enemy : enemies_) {
        if (enemy.health <= 0) continue;
        enemy.laserTime = std::max(0.0f, enemy.laserTime - dt);
        enemy.castFlash = std::max(0.0f, enemy.castFlash - dt);

        // Only the CLOSEST player to the enemy can block its forward movement.
        const PlayerSnapshot* blocker = nullptr;
        for (const PlayerSnapshot& p : views) {
            if (p.lane != enemy.lane) continue;
            const float contact = enemy.halfWidth() + p.halfWidth;
            const float gap = enemy.x - p.x;
            const bool touching = std::abs(gap) <= contact + 0.5f;
            const bool aboutToMeet = gap > 0.0f && gap <= contact + enemy.speed * dt;
            if ((touching || aboutToMeet) && (!blocker || p.x > blocker->x))
                blocker = &p;
        }

        if (blocker) {
            // no player/enemy passing through one another on contact.
            enemy.x = blocker->x + enemy.halfWidth() + blocker->halfWidth;
            players.setEngaged(blocker->id, true);
        } else {
            enemy.x -= enemy.speed * dt;
            enemy.walkTime += dt;
        }

        // Draw an anticipatory arm pose before the attack is actually released.
        enemy.attackTimer -= dt;
        const float windup = std::clamp(
            (WINDUP_SECONDS - enemy.attackTimer) / WINDUP_SECONDS, 0.0f, 1.0f);
        enemy.attackPose = std::max(windup,
            enemy.castFlash / 0.25f);

        if (enemy.attackTimer <= 0.0f) {
            enemy.castFlash = 0.25f;
            enemy.attackPose = 1.0f;

            if (enemy.kind == EnemyKind::Medusa) {
                // HIGH ranged beam. Ducking avoids it.
                enemy.laserTime = 0.25f;
                audio::play(audio::Sfx::Beam);
                const float fromX = enemy.x - enemy.halfWidth();
                const float toX = std::max(FIELD_X0, fromX - LASER_REACH);
                players.applyEnemyAttack(enemy.lane, fromX, toX,
                                         enemy.attackDamage, AttackHeight::High, 5.0f);
            } else if (enemy.kind == EnemyKind::Mummy) {
                // a LOW tornado travels to the left. Jumping avoids it.
                audio::play(audio::Sfx::Tornado);
                projectiles_.push_back({EnemyKind::Mummy, enemy.lane,
                                        enemy.x - 25.0f, enemy.footY() + 25.0f,
                                        enemy.attackDamage, 0.0f});
            } else {
                // Pharaoh has a MELEE strike, not a travelling projectile.
                const float fromX = enemy.x - enemy.halfWidth();
                const float toX = std::max(FIELD_X0, fromX - MELEE_REACH);
                const auto hit = players.applyEnemyAttack(
                    enemy.lane, fromX, toX, enemy.attackDamage, AttackHeight::Melee, 4.0f);

                audio::play(audio::Sfx::Thunder);
                // Lightning is only a VISUAL; it cannot deal damage again.
                const float strikeX = hit ? hit->x : std::max(FIELD_X0, enemy.x - 65.0f);
                projectiles_.push_back({EnemyKind::Pharaoh, enemy.lane,
                                        strikeX, enemy.footY() + 60.0f, 0, 0.0f});
            }
            enemy.attackTimer = attackInterval(enemy.kind);
        }

        if (enemy.x <= FIELD_X0) {
            game.lose(); // Freezes gameplay through the existing Game state machine.
            return;
        }
    }

    // ========== MOVING TORNADO COLLISIONS ==========
    for (EnemyProjectile& p : projectiles_) {
        p.age += dt;
        if (p.kind != EnemyKind::Mummy) continue; // Pharaoh's bolt is just a flash.

        const float oldX = p.x;
        p.x -= 225.0f * dt;
        const auto hit = players.applyEnemyAttack(
            p.lane, oldX, p.x, p.damage, AttackHeight::Low, 12.0f);
        if (hit) p.age = 99.0f; // One tornado cannot deal damage twice.
    }
    projectiles_.erase(std::remove_if(projectiles_.begin(), projectiles_.end(), [](const EnemyProjectile& p) {
        if (p.kind == EnemyKind::Pharaoh) return p.age >= 0.25f;
        return p.age >= 99.0f || p.age >= 5.0f || p.x < FIELD_X0;
    }), projectiles_.end());

    // ========== PLAYER ATTACKS AGAINST ENEMIES ==========
    // arrows travel left-to-right, so resolve ENEMIES LEFT-TO-RIGHT.
    // This prevents an arrow from being consumed by a farther enemy first.
    std::vector<std::size_t> order(enemies_.size());
    std::iota(order.begin(), order.end(), std::size_t{0});
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        if (enemies_[a].lane != enemies_[b].lane)
            return enemies_[a].lane < enemies_[b].lane;
        return enemies_[a].x < enemies_[b].x;
    });

    for (std::size_t index : order) {
        const Enemy& enemy = enemies_[index];
        if (enemy.health <= 0) continue;
        const int damage = players.applyPlayerAttacksToEnemy(
            enemy.lane, enemy.x - enemy.halfWidth(), enemy.x + enemy.halfWidth());
        if (damage > 0) damageEnemy(index, damage);
    }
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) {
        return e.health <= 0;
    }), enemies_.end());

    // Recalculate contact after deaths so the surviving player resumes moving.
    players.setEngagements(enemies_, 0.0f);

    // ========== PRESERVE THE TEAM'S THREE-WAVE PROGRESSION ==========
#ifdef TEMPLE_COMBAT_TESTS
    if (manualTestMode_) return; // Test matchups must not spawn unrelated enemies.
#endif
    // No new wave until ALL scheduled enemies have spawned and the field is clear.
    if (spawned_ == waveTarget(wave_) && enemies_.empty() && projectiles_.empty()) {
        if (wave_ == 3) {
            game.win();
            return;
        }
        ++wave_;
        spawned_ = 0;
        spawnClock_ = 2.0f;
    }

    if (spawned_ < waveTarget(wave_)) {
        spawnClock_ -= dt;
        if (spawnClock_ <= 0.0f)
            spawnClock_ = spawnNext() ? 2.2f : 0.25f;
    }
}

void EnemySystem::drawEnemies(Renderer& r) const {
    for (const Enemy& enemy : enemies_) {
        switch (enemy.kind) {
            case EnemyKind::Medusa: drawMedusa(r, enemy); break;
            case EnemyKind::Mummy: drawMummy(r, enemy); break;
            case EnemyKind::Pharaoh: drawPharaoh(r, enemy); break;
        }
    }
}

void EnemySystem::drawHealthBar(Renderer& r, const Enemy& enemy) const {
    const float width = 70.0f;
    const float height = 8.0f;
    const float gap = 2.0f;
    const float bottom = enemy.footY() + 130.0f;
    const float first = enemy.x - width * 0.5f;
    const float segmentW = (width - (enemy.maxHealth - 1) * gap) / enemy.maxHealth;

    glm::vec4 color(0.22f, 0.84f, 0.66f, 1.0f);
    if (enemy.kind == EnemyKind::Pharaoh) color = {1.0f, 0.77f, 0.23f, 1.0f};
    if (enemy.kind == EnemyKind::Medusa) color = {1.0f, 0.29f, 0.30f, 1.0f};

    r.rectBL({first - 3, bottom - 3}, {width + 6, height + 6},
             {0.12f, 0.10f, 0.11f, 0.92f});
    for (int i = 0; i < enemy.maxHealth; ++i) {
        const glm::vec4 fill = i < enemy.health ? color : glm::vec4(0.28f, 0.24f, 0.22f, 1.0f);
        r.rectBL({first + i * (segmentW + gap), bottom}, {segmentW, height}, fill);
    }
}

void EnemySystem::drawAttacksAndHealth(Renderer& r) const {
    for (const EnemyProjectile& p : projectiles_) {
        if (p.kind == EnemyKind::Mummy) drawMummyTornado(r, p);
        else if (p.kind == EnemyKind::Pharaoh) drawPharaohLightning(r, p);
    }
    for (const Enemy& e : enemies_) {
        if (e.kind == EnemyKind::Medusa) drawMedusaLaser(r, e);
        drawHealthBar(r, e);
    }
}
