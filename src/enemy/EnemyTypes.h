
#pragma once

#include "Config.h"
#include <glm/glm.hpp>

// Assignment mapping: Easy=Medusa, Medium=Mummy, Hard=Pharaoh.
enum class EnemyKind { Mummy, Pharaoh, Medusa };

struct Enemy {
    EnemyKind kind;
    int lane;
    float x;
    int health;
    int maxHealth;
    int attackDamage;
    float speed;

    float walkTime = 0.0f;
    float attackTimer = 0.0f;
    float laserTime = 0.0f;   // How long Medusa's red laser stays visible.
    float attackPose = 0.0f;  // 0 = normal arms, 1 = fully extended to cast.
    float castFlash = 0.0f;   // Keep the arm extension briefly after a strike.

    float footY() const { return laneBottomY(lane) + 8.0f; }
    float halfWidth() const { return 23.0f; }
};

// Only Mummy's tornado moves as a damaging projectile.
// A Pharaoh entry is a brief lightning VISUAL; damage happens at melee range.
struct EnemyProjectile {
    EnemyKind kind;
    int lane;
    float x;
    float y;
    int damage = 1;
    float age = 0.0f;
};

inline glm::vec4 enemyDamageColor(glm::vec4 normal, const Enemy& enemy) {
    const float lost = 1.0f - static_cast<float>(enemy.health) / enemy.maxHealth;
    return glm::mix(normal, glm::vec4(0.95f, 0.10f, 0.12f, normal.a), lost * 0.70f);
}
