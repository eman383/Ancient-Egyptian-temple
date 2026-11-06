
#include "Wadjet.h"
#include <cmath>

// Wadjet: the cobra goddess of Lower Egypt, woken as a corrupted guardian.
// Easy enemy: walks at normal speed and fires the "Eye of Ra" beam from above
// (head height), so players must DUCK to avoid it.
namespace {
const glm::vec4 SCALE_GREEN(0.20f, 0.50f, 0.33f, 1.0f);   // cobra body / skin
const glm::vec4 SCALE_DARK (0.10f, 0.30f, 0.20f, 1.0f);
const glm::vec4 HOOD_LIGHT (0.78f, 0.70f, 0.38f, 1.0f);   // pale hood belly scales
const glm::vec4 LINEN      (0.86f, 0.80f, 0.64f, 1.0f);   // sheath dress
const glm::vec4 CROWN_RED  (0.72f, 0.12f, 0.10f, 1.0f);   // deshret, red crown of Lower Egypt
const glm::vec4 GOLD       (0.94f, 0.72f, 0.22f, 1.0f);
const glm::vec4 LAPIS      (0.13f, 0.27f, 0.62f, 1.0f);
const glm::vec4 EYE_FIRE   (1.00f, 0.55f, 0.08f, 1.0f);
}

void drawWadjet(Renderer& r, const Enemy& enemy) {
    const glm::vec4 skin  = enemyDamageColor(SCALE_GREEN, enemy);
    const glm::vec4 dark  = enemyDamageColor(SCALE_DARK, enemy);
    const glm::vec4 belly = enemyDamageColor(HOOD_LIGHT, enemy);
    const glm::vec4 dress = enemyDamageColor(LINEN, enemy);
    const glm::vec4 crown = enemyDamageColor(CROWN_RED, enemy);

    const float stride = std::sin(enemy.walkTime * 6.0f) * 14.0f;
    const float bob    = std::fabs(std::sin(enemy.walkTime * 6.0f)) * 2.0f;
    const float sway   = std::sin(enemy.walkTime * 3.0f) * 3.0f;

    r.push();
    r.translate({enemy.x, enemy.footY()});
    r.scale({-1.0f, 1.0f});                       // face left

    r.ellipse({0, -3}, {34, 5}, {0, 0, 0, 0.26f}); // shadow

    // Serpent tail trailing behind (sways as she walks)
    r.push();
    r.translate({-18, 8});
    r.rotate(sway);
    r.ellipse({-14, 0}, {18, 6}, dark);
    r.ellipse({-30, 3}, {10, 4}, dark);
    r.triangle({-42, 4}, {8, 8}, dark);
    r.pop();

    // Scaled legs in walking motion
    for (int side : {-1, 1}) {
        r.push();
        r.translate({side * 10.0f, 44});
        r.rotate(stride * side);
        r.rect({0, -18}, {14, 36}, skin);
        r.rect({0, -6}, {15, 3}, dark);           // scale bands
        r.rect({0, -20}, {15, 3}, dark);
        r.rect({4, -37}, {22, 6}, dark);          // clawed feet
        r.pop();
    }

    r.push();
    r.translate({0, bob});

    // Linen sheath dress with a lapis-and-gold belt
    r.polygon({{-22, 40}, {22, 40}, {18, 92}, {-18, 92}}, dress);
    r.polygon({{-22, 40}, {-8, 40}, {-6, 92}, {-18, 92}},
              {dress.r * 0.82f, dress.g * 0.82f, dress.b * 0.82f, 1.0f});
    r.rect({0, 60}, {44, 7}, GOLD);
    r.rect({0, 60}, {12, 7}, LAPIS);

    // Usekh broad collar: gold and lapis bands
    r.ellipse({0, 90}, {22, 9}, GOLD);
    r.ellipse({0, 90}, {17, 6}, LAPIS);
    r.ellipse({0, 90}, {12, 4}, GOLD);

    // Back arm
    r.rect({-24, 74}, {12, 32}, skin);

    // Front arm: raised toward the player while she casts
    r.push();
    r.translate({20, 84});
    r.rotate(-15.0f + 35.0f * enemy.attackPose + stride * 0.1f);
    r.rect({13, -4}, {28, 11}, skin);
    r.rect({20, -4}, {6, 12}, GOLD);              // bracelet
    for (int i = 0; i < 3; i++)                   // claws
        r.triangle({30.0f + i * 4.0f, -12}, {5, 9}, dark);
    r.pop();

    // Cobra hood flaring behind the head; it widens while she casts
    const float flare = 1.0f + 0.18f * enemy.attackPose;
    r.push();
    r.translate({-2, 104});
    r.scale({flare, 1.0f});
    r.ellipse({0, 0}, {24, 26}, dark);
    r.ellipse({2, -2}, {17, 21}, belly);
    for (int i = 0; i < 3; i++)                   // hood markings
        r.rect({2, -12.0f + i * 9.0f}, {22.0f - i * 4.0f, 2.5f}, dark);
    r.pop();

    // Cobra head with a forward snout
    r.ellipse({4, 103}, {13, 12}, skin);
    r.polygon({{8, 108}, {24, 102}, {24, 96}, {8, 94}}, skin);   // snout
    r.rect({20, 95}, {8, 1.5f}, dark);                           // mouth line
    r.triangle({26, 92}, {3, 6}, {0.95f, 0.92f, 0.85f, 1.0f});   // fang

    // Red crown of Lower Egypt with a golden coil
    r.polygon({{-8, 112}, {14, 112}, {12, 124}, {-6, 124}}, crown);
    r.rect({-5, 132}, {7, 18}, crown);                           // tall back
    r.ellipse({12, 125}, {5, 3}, GOLD);                          // coil tip

    // Glowing eyes (the Eye of Ra beam fires from here)
    r.ellipse({14, 104}, {4.0f, 3.0f}, EYE_FIRE);
    r.ellipse({16, 98}, {3.2f, 2.4f}, EYE_FIRE);
    r.circle({15, 104}, 1.5f, {1.0f, 0.95f, 0.7f, 1.0f});

    r.pop();
    r.pop();
}

// The Eye of Ra: a HIGH beam at head height, matching the 450px reach in EnemySystem.cpp.
void drawWadjetLaser(Renderer& r, const Enemy& enemy) {
    if (enemy.laserTime <= 0.0f) return;

    r.push();
    r.translate({enemy.x, enemy.footY()});
    r.scale({-1.0f, 1.0f});

    const float opacity = enemy.laserTime / 0.25f;
    const float length = 450.0f;

    for (float eyeY : {104.0f, 98.0f}) {
        r.circle({18, eyeY}, 6, {1.0f, 0.6f, 0.1f, opacity * 0.7f});
        r.rect({18.0f + length * 0.5f, eyeY}, {length, 11}, {1.0f, 0.45f, 0.05f, opacity * 0.3f});
        r.rect({18.0f + length * 0.5f, eyeY}, {length, 3.5f}, {1.0f, 0.75f, 0.25f, opacity});
        r.circle({18.0f + length, eyeY}, 4, {1.0f, 0.95f, 0.6f, opacity});
    }
    r.pop();
}
