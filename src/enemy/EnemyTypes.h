
#pragma once

#include "Config.h"
#include <glm/glm.hpp>

// Our three enemy characters
enum class EnemyKind {
    Mummy,
    Pharaoh,
    Medusa
};

// Information about one enemy
struct Enemy {
    EnemyKind kind;

    int lane;          // Which lane the enemy is walking on
    float x;           // Horizontal position

    int health;
    int maxHealth;
    int attackDamage;

    float speed;       // Movement speed in pixels per second

    float walkTime = 0.0f;
    float attackTimer = 0.0f;
    float laserTime = 0.0f;

    // Position the enemy's feet above the lane ground
    float footY() const {
        return laneBottomY(lane) + 8.0f;
    }
};

// Information about a projectile
struct EnemyProjectile {
    EnemyKind kind;

    int lane;
    float x;
    float y;

    int damage = 1;
    float age = 0.0f;
};

// Gradually change the model's color toward red
// when its health decreases.
inline glm::vec4 enemyDamageColor(
    glm::vec4 normal,
    const Enemy& enemy
) {
    float lost = 1.0f -
        static_cast<float>(enemy.health) / enemy.maxHealth;

    return glm::mix(
        normal,
        glm::vec4(0.95f, 0.10f, 0.12f, normal.a),
        lost * 0.70f
    );
}
