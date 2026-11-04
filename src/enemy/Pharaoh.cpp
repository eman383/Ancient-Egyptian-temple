#include "Pharaoh.h"
#include <cmath>

namespace {
const glm::vec4 BLUE(0.15f, 0.34f, 0.62f, 1.0f);
const glm::vec4 BLUE_DARK(0.08f, 0.20f, 0.44f, 1.0f);
const glm::vec4 GOLD(0.97f, 0.78f, 0.24f, 1.0f);
const glm::vec4 GOLD_DARK(0.66f, 0.44f, 0.12f, 1.0f);
const glm::vec4 SKIN(0.72f, 0.43f, 0.26f, 1.0f);
const glm::vec4 WHITE(0.92f, 0.85f, 0.66f, 1.0f);

// Draw a lightning segment using a rotated rectangle
void boltSegment(
    Renderer& r,
    glm::vec2 a,
    glm::vec2 b,
    float width,
    glm::vec4 color
) {
    const glm::vec2 d = b - a;
    const float length = std::sqrt(d.x * d.x + d.y * d.y);
    const float angle =
        std::atan2(d.y, d.x) * 180.0f / 3.14159265f;

    r.push();

    r.translate((a + b) * 0.5f);
    r.rotate(angle);
    r.rect({0, 0}, {length, width}, color);

    r.pop();
}
}

// Draw the medium enemy
void drawPharaoh(Renderer& r, const Enemy& enemy) {
    const glm::vec4 blue = enemyDamageColor(BLUE, enemy);
    const glm::vec4 dark = enemyDamageColor(BLUE_DARK, enemy);
    const glm::vec4 gold = enemyDamageColor(GOLD, enemy);

    const float step = std::sin(enemy.walkTime * 6.0f) * 12.0f;
    const float bob =
        std::fabs(std::sin(enemy.walkTime * 6.0f)) * 1.5f;

    r.push();

    r.translate({enemy.x, enemy.footY()});
    r.scale({-1.0f, 1.0f});

    // Shadow
    r.ellipse({0, -3}, {32, 5}, {0, 0, 0, 0.22f});

    // Legs and sandals
    for (int side : {-1, 1}) {
        r.push();

        r.translate({side * 12.0f, 41.0f});
        r.rotate(side * step);

        r.rect({0, -17}, {16, 37},
               enemyDamageColor(SKIN, enemy));
        r.rect({0, -18}, {18, 5}, gold);
        r.rect({6, -35}, {24, 7}, gold);

        r.pop();
    }

    r.push();
    r.translate({0, bob});

    // Royal skirt
    r.polygon({
        {-27, 39}, {27, 39},
        {20, 71}, {-20, 71}
    }, WHITE);

    // Blue cloak
    r.polygon({
        {-23, 44}, {-7, 44},
        {-4, 70}, {-21, 70}
    }, blue);

    r.polygon({
        {8, 44}, {24, 44},
        {20, 70}, {6, 70}
    }, blue);

    // Belt
    r.rect({0, 68}, {46, 7}, gold);
    r.circle({0, 68}, 5.0f, dark);

    // Upper body
    r.polygon({
        {-21, 71}, {23, 71},
        {19, 95}, {-19, 95}
    }, blue);

    r.rect({0, 88}, {43, 5}, gold);
    r.rect({0, 81}, {35, 3}, dark);

    // Right arm, holding the Key of Life
    r.push();

    r.translate({20, 85});
    r.rotate(-8.0f + step * 0.15f);

    r.rect({13, -8}, {26, 12},
           enemyDamageColor(SKIN, enemy));
    r.rect({3, -5}, {10, 16}, gold);
    r.circle({27, -8}, 6.0f,
             enemyDamageColor(SKIN, enemy));

    // Ankh: vertical stem
    r.rect({38, 2}, {6, 39}, gold);

    // Ankh: horizontal crossbar
    r.rect({38, 9}, {26, 6}, gold);

    // Ankh: oval loop with a darker opening
    r.ellipse({38, 29}, {10, 12}, gold);
    r.ellipse({38, 29}, {5, 7}, dark);

    r.pop();

    // Other arm
    r.rect({-23, 82}, {13, 21}, blue);
    r.circle({-23, 68}, 7.0f,
             enemyDamageColor(SKIN, enemy));

    // Striped headcloth
    r.polygon({
        {-29, 90}, {-16, 89},
        {-17, 118}, {-24, 119}
    }, blue);

    r.polygon({
        {17, 90}, {30, 90},
        {24, 119}, {17, 119}
    }, blue);

    for (int y = 93; y <= 115; y += 10) {
        r.rect({-22, static_cast<float>(y)}, {11, 3}, gold);
        r.rect({23, static_cast<float>(y)}, {11, 3}, gold);
    }

    // Face
    r.ellipse({0, 105}, {17, 17},
              enemyDamageColor(SKIN, enemy));

    // Crown
    r.rect({0, 119}, {42, 9}, blue);
    r.rect({0, 121}, {38, 3}, gold);

    // Cobra ornament
    r.triangle({0, 121}, {9, 8}, gold);

    // Eye and mouth
    r.ellipse({10, 108}, {3.8f, 2.3f},
              {0.11f, 0.08f, 0.08f, 1.0f});

    r.rect({13, 101}, {6, 2}, GOLD_DARK);

    // Collar
    r.rect({0, 91}, {14, 4}, gold);

    // Brief lightning flash around the Ankh
    if (enemy.attackTimer > 1.60f) {
        boltSegment(
            r, {57, 64}, {66, 46}, 6.0f,
            {1.0f, 0.87f, 0.28f, 0.45f}
        );

        boltSegment(
            r, {66, 46}, {73, 27}, 4.0f,
            {1.0f, 0.98f, 0.72f, 1.0f}
        );

        r.circle(
            {73, 27}, 5.0f,
            {1.0f, 0.92f, 0.35f, 0.65f}
        );
    }

    r.pop();
    r.pop();
}

// Draw the Pharaoh's LOW lightning projectile
void drawPharaohLightning(
    Renderer& r,
    const EnemyProjectile& bolt
) {
    r.push();
    r.translate({bolt.x, bolt.y});

    const float flash =
        0.75f + 0.25f * std::sin(bolt.age * 30.0f);

    const glm::vec4 glow(
        1.0f, 0.89f, 0.24f, 0.37f
    );

    const glm::vec4 core(
        1.0f, 0.98f, flash, 1.0f
    );

    // Zigzag lightning path
    const glm::vec2 path[] = {
        {-30, 11},
        {-11, 2},
        {-17, -11},
        {1, 0},
        {9, -12},
        {30, -4}
    };

    // Wider glow
    for (int i = 0; i < 5; i++) {
        boltSegment(
            r, path[i], path[i + 1],
            10.0f, glow
        );
    }

    // Thin lightning core
    for (int i = 0; i < 5; i++) {
        boltSegment(
            r, path[i], path[i + 1],
            4.0f, core
        );
    }

    r.circle({-30, 11}, 4.5f, core);
    r.circle({30, -4}, 4.0f, core);

    r.pop();
}
