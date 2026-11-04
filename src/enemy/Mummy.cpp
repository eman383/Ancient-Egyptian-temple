#include "Mummy.h"
#include <cmath>

namespace {
const glm::vec4 WRAP(0.91f, 0.85f, 0.67f, 1.0f);
const glm::vec4 WRAP_DARK(0.63f, 0.55f, 0.40f, 1.0f);
const glm::vec4 SHADOW(0.14f, 0.10f, 0.08f, 0.28f);
const glm::vec4 EYE(0.08f, 0.56f, 0.60f, 1.0f);
}

// Draw the easy enemy using basic shapes
void drawMummy(Renderer& r, const Enemy& enemy) {
    const glm::vec4 cloth = enemyDamageColor(WRAP, enemy);
    const glm::vec4 seams = enemyDamageColor(WRAP_DARK, enemy);

    // Alternate the legs to simulate walking
    const float step = std::sin(enemy.walkTime * 6.0f) * 13.0f;
    const float bob = std::fabs(std::sin(enemy.walkTime * 6.0f)) * 2.0f;

    r.push();

    // Position at the correct lane
    r.translate({enemy.x, enemy.footY()});

    // Face left
    r.scale({-1.0f, 1.0f});

    // Shadow under the mummy
    r.ellipse({0, -3}, {29, 5}, SHADOW);

    // Legs
    for (int side : {-1, 1}) {
        r.push();

        r.translate({side * 12.0f, 43.0f});
        r.rotate(step * side);

        r.rect({0, -18}, {17, 38}, cloth);
        r.rect({0, -11}, {18, 4}, seams);
        r.rect({0, -24}, {18, 3}, seams);

        // Shoes
        r.rect({5, -36}, {23, 8}, cloth);

        r.pop();
    }

    r.push();
    r.translate({0, bob});

    // Torso
    r.polygon({
        {-23, 43}, {23, 43}, {26, 85},
        {16, 94}, {-18, 94}, {-26, 84}
    }, cloth);

    // Bandages
    r.polygon({
        {-24, 55}, {23, 63}, {23, 69}, {-24, 61}
    }, seams);

    r.polygon({
        {-23, 76}, {21, 83}, {23, 88}, {-22, 81}
    }, seams);

    r.polygon({
        {-23, 47}, {11, 43}, {24, 48}, {-12, 53}
    }, seams);

    r.rect({0, 42}, {50, 5}, cloth);

    // Left arm
    r.push();
    r.translate({-23, 82});
    r.rotate(-15.0f - step * 0.25f);

    r.rect({-8, -12}, {14, 34}, cloth);
    r.rect({-8, -15}, {16, 4}, seams);

    r.pop();

    // Right arm, reaching toward the player
    r.push();
    r.translate({20, 83});
    r.rotate(-6.0f + step * 0.30f);

    r.rect({17, 0}, {37, 13}, cloth);
    r.rect({18, 0}, {5, 14}, seams);
    r.rect({34, 0}, {5, 14}, seams);
    r.ellipse({39, 0}, {8, 6}, cloth);

    r.pop();

    // Head
    r.ellipse({0, 108}, {17, 16}, cloth);

    // Head bandages
    r.rect({0, 120}, {26, 4}, seams);
    r.rect({-3, 116}, {29, 3}, seams);
    r.rect({-4, 105}, {30, 3}, seams);

    // Eye socket and glowing eye
    r.ellipse({8, 110}, {5, 6},
              {0.14f, 0.12f, 0.10f, 1.0f});
    r.circle({9, 110}, 2.5f, EYE);

    // Mouth
    r.rect({9, 101}, {9, 2}, seams);

    r.pop();
    r.pop();
}

// Draw a HIGH tornado projectile
void drawMummyTornado(
    Renderer& r,
    const EnemyProjectile& wind
) {
    r.push();
    r.translate({wind.x, wind.y});

    const float spin = wind.age * 11.0f;

    const glm::vec4 dust(
        0.85f, 0.94f, 0.87f, 0.52f
    );

    const glm::vec4 air(
        0.97f, 1.00f, 0.97f, 0.86f
    );

    // Tornado funnel
    r.polygon({
        {-22, 22}, {22, 22},
        {12, -23}, {-9, -23}
    }, dust);

    // Swirling rings
    for (int i = 0; i < 5; i++) {
        float y = -20.0f + i * 11.0f;
        float x = std::sin(spin + i * 1.7f)
                * (4.0f + i * 1.5f);
        float w = 12.0f + i * 5.0f;

        r.ellipse(
            {x, y},
            {w, 3.5f},
            (i % 2 == 0) ? air : dust
        );
    }

    // Small particles of sand
    r.circle(
        {22 + std::sin(spin) * 4.0f, 15},
        4.0f,
        air
    );

    r.circle({-23, -6}, 3.5f, dust);

    r.pop();
}
