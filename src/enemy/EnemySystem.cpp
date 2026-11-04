
#include "EnemySystem.h"
#include "Mummy.h"
#include "Pharaoh.h"
#include "Medusa.h"

#include <algorithm>

namespace {

// Movement speeds in virtual pixels per second
constexpr float NORMAL_SPEED = 38.0f;
constexpr float RUN_SPEED = 72.0f;

// Time between attacks for each enemy
float attackInterval(EnemyKind kind) {
    if (kind == EnemyKind::Mummy)
        return 2.60f;

    if (kind == EnemyKind::Pharaoh)
        return 1.80f;

    return 1.15f;
}

}

// Create an enemy at the RIGHT edge of the chosen lane
void EnemySystem::spawn(EnemyKind kind, int lane) {
    // Make sure the lane exists
    if (lane < 0 || lane >= LANE_COUNT)
        return;

    bool hard = kind == EnemyKind::Medusa;

    Enemy enemy{
        kind,
        lane,
        FIELD_X1,                  // X = 1600
        hard ? 7 : 5,              // Health
        hard ? 7 : 5,              // Maximum health
        hard ? 2 : 1,              // Attack damage
        hard ? RUN_SPEED : NORMAL_SPEED
    };

    // Start attacks at slightly different times
    if (kind == EnemyKind::Mummy)
        enemy.attackTimer = 0.35f;
    else if (kind == EnemyKind::Pharaoh)
        enemy.attackTimer = 0.65f;
    else
        enemy.attackTimer = 0.85f;

    enemies_.push_back(enemy);
}

// Spawn one character of each type for testing
void EnemySystem::reset() {
    enemies_.clear();
    projectiles_.clear();

    spawn(EnemyKind::Mummy, 0);    // Top lane
    spawn(EnemyKind::Pharaoh, 2);  // Middle lane
    spawn(EnemyKind::Medusa, 4);   // Bottom lane
}

// Reduce an enemy's remaining health
void EnemySystem::damageEnemy(
    std::size_t index,
    int amount
) {
    if (index >= enemies_.size() || amount <= 0)
        return;

    Enemy& enemy = enemies_[index];

    enemy.health = std::max(
        0,
        enemy.health - amount
    );
}

// Testing function: damage the first enemy of a type
void EnemySystem::damageFirstOfType(EnemyKind kind) {
    for (std::size_t i = 0; i < enemies_.size(); i++) {
        if (enemies_[i].kind == kind) {
            damageEnemy(i, 1);
            return;
        }
    }
}

// Called every gameplay frame
void EnemySystem::update(Game& game, float dt) {

    // Temporary keyboard controls for testing health
    if (game.input.keyJustPressed(GLFW_KEY_7))
        damageFirstOfType(EnemyKind::Mummy);

    if (game.input.keyJustPressed(GLFW_KEY_8))
        damageFirstOfType(EnemyKind::Pharaoh);

    if (game.input.keyJustPressed(GLFW_KEY_9))
        damageFirstOfType(EnemyKind::Medusa);

    // Remove defeated enemies
    enemies_.erase(
        std::remove_if(
            enemies_.begin(),
            enemies_.end(),
            [](const Enemy& e) {
                return e.health <= 0;
            }
        ),
        enemies_.end()
    );

    // Update each surviving enemy
    for (Enemy& enemy : enemies_) {

        // Update the walking animation
        enemy.walkTime += dt;

        // MOVE LEFT
        enemy.x -= enemy.speed * dt;

        // Reduce time remaining for Medusa's laser
        enemy.laserTime = std::max(
            0.0f,
            enemy.laserTime - dt
        );

        // Update attack timer
        enemy.attackTimer -= dt;

        // Time to attack?
        if (enemy.attackTimer <= 0.0f) {

            if (enemy.kind == EnemyKind::Mummy) {

                // Spawn a HIGH tornado projectile
                projectiles_.push_back({
                    enemy.kind,
                    enemy.lane,
                    enemy.x - 49.0f,
                    enemy.footY() + 104.0f,
                    enemy.attackDamage
                });

            } else if (enemy.kind == EnemyKind::Pharaoh) {

                // Spawn a LOW lightning projectile
                projectiles_.push_back({
                    enemy.kind,
                    enemy.lane,
                    enemy.x - 58.0f,
                    enemy.footY() + 30.0f,
                    enemy.attackDamage
                });

            } else {

                // Medusa activates her eye lasers
                enemy.laserTime = 0.25f;
            }

            // Schedule the next attack
            enemy.attackTimer += attackInterval(enemy.kind);
        }

        // Enemy reached the temple boundary
        if (enemy.x <= FIELD_X0) {
            game.lose();
            return;
        }
    }

    // Move projectiles left
    for (EnemyProjectile& projectile : projectiles_) {
        projectile.age += dt;

        float projectileSpeed =
            projectile.kind == EnemyKind::Mummy
            ? 225.0f
            : 280.0f;

        projectile.x -= projectileSpeed * dt;
    }

    // Remove projectiles that reach the left boundary
    projectiles_.erase(
        std::remove_if(
            projectiles_.begin(),
            projectiles_.end(),
            [](const EnemyProjectile& p) {
                return p.x < FIELD_X0;
            }
        ),
        projectiles_.end()
    );
}

// Draw all enemies
void EnemySystem::drawEnemies(Renderer& r) const {
    for (const Enemy& enemy : enemies_) {

        switch (enemy.kind) {
            case EnemyKind::Mummy:
                drawMummy(r, enemy);
                break;

            case EnemyKind::Pharaoh:
                drawPharaoh(r, enemy);
                break;

            case EnemyKind::Medusa:
                drawMedusa(r, enemy);
                break;
        }
    }
}

// Draw a segmented health bar above an enemy
void EnemySystem::drawHealthBar(
    Renderer& r,
    const Enemy& enemy
) const {
    const float width = 70.0f;
    const float height = 8.0f;
    const float gap = 2.0f;

    const float bottom = enemy.footY() + 130.0f;
    const float first = enemy.x - width * 0.5f;

    const float segmentW =
        (width - (enemy.maxHealth - 1) * gap)
        / enemy.maxHealth;

    // Default mummy health bar color
    glm::vec4 color(
        0.22f, 0.84f, 0.66f, 1.0f
    );

    // Pharaoh health bar color
    if (enemy.kind == EnemyKind::Pharaoh) {
        color = {1.0f, 0.77f, 0.23f, 1.0f};
    }

    // Medusa health bar color
    if (enemy.kind == EnemyKind::Medusa) {
        color = {1.0f, 0.29f, 0.30f, 1.0f};
    }

    // Dark health bar background
    r.rectBL(
        {first - 3, bottom - 3},
        {width + 6, height + 6},
        {0.12f, 0.10f, 0.11f, 0.92f}
    );

    // Health segments
    for (int i = 0; i < enemy.maxHealth; i++) {
        glm::vec4 fill;

        if (i < enemy.health) {
            fill = color;
        } else {
            fill = glm::vec4(
                0.28f, 0.24f, 0.22f, 1.0f
            );
        }

        r.rectBL(
            {
                first + i * (segmentW + gap),
                bottom
            },
            {segmentW, height},
            fill
        );
    }
}

// Draw projectiles, eye lasers and health bars
void EnemySystem::drawAttacksAndHealth(
    Renderer& r
) const {
    // Draw flying attacks
    for (const EnemyProjectile& projectile : projectiles_) {
        if (projectile.kind == EnemyKind::Mummy) {
            drawMummyTornado(r, projectile);
        } else if (projectile.kind == EnemyKind::Pharaoh) {
            drawPharaohLightning(r, projectile);
        }
    }

    // Draw Medusa's beams and each enemy's health
    for (const Enemy& enemy : enemies_) {
        if (enemy.kind == EnemyKind::Medusa) {
            drawMedusaLaser(r, enemy);
        }

        drawHealthBar(r, enemy);
    }
}
