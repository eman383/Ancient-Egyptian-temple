#include "Medusa.h"
#include <cmath>

namespace {
const glm::vec4 SKIN(0.35f, 0.64f, 0.40f, 1.0f);
const glm::vec4 SNAKE(0.12f, 0.44f, 0.30f, 1.0f);
const glm::vec4 SNAKE_LITE(0.48f, 0.77f, 0.44f, 1.0f);
const glm::vec4 ARMOR(0.34f, 0.22f, 0.43f, 1.0f);
const glm::vec4 GOLD(0.94f, 0.72f, 0.22f, 1.0f);
const glm::vec4 LASER(1.00f, 0.09f, 0.12f, 1.0f);
}

// Draw Medusa, the hardest enemy
void drawMedusa(Renderer& r, const Enemy& enemy) {
    const glm::vec4 green = enemyDamageColor(SKIN, enemy);
    const glm::vec4 snake = enemyDamageColor(SNAKE, enemy);
    const glm::vec4 armor = enemyDamageColor(ARMOR, enemy);

    // Faster running animation
    const float run = std::sin(enemy.walkTime * 10.0f) * 22.0f;
    const float bob =
        std::fabs(std::sin(enemy.walkTime * 10.0f)) * 2.0f;

    r.push();

    // Correct position on the lane
    r.translate({enemy.x, enemy.footY()});

    // Face left
    r.scale({-1.0f, 1.0f});

    // Shadow
    r.ellipse({0, -3}, {34, 5}, {0, 0, 0, 0.26f});

    // Running legs
    for (int side : {-1, 1}) {
        r.push();

        r.translate({side * 11.0f, 44});
        r.rotate(run * side);

        r.rect({0, -18}, {16, 36}, green);
        r.rect({0, -24}, {18, 14}, armor);

        // Pointed boots
        r.triangle({6, -36}, {26, 13}, armor);

        r.pop();
    }

    r.push();
    r.translate({0, bob});

    // Armored torso
    r.polygon({
        {-23, 44}, {23, 44},
        {20, 92}, {-20, 92}
    }, armor);

    // Lower armor
    r.polygon({
        {-25, 48}, {25, 48},
        {18, 61}, {-18, 61}
    }, snake);

    // Belt
    r.rect({0, 60}, {47, 7}, GOLD);
    r.circle({0, 60}, 5, green);

    // Chest decoration
    r.polygon({
        {-13, 71}, {0, 63},
        {13, 71}, {0, 84}
    }, GOLD);

    r.polygon({
        {-17, 90}, {0, 83},
        {17, 90}, {15, 94}, {-15, 94}
    }, snake);

    // Left arm
    r.rect({-26, 75}, {14, 35}, green);
    r.circle({-26, 55}, 7, green);

    // Right arm and claws
    r.push();

    r.translate({21, 84});
    r.rotate(-12.0f + run * 0.1f);

    r.rect({12, -10}, {26, 13}, green);
    r.rect({23, -11}, {9, 5}, GOLD);

    for (int i = 0; i < 3; i++) {
        r.triangle(
            {30 + i * 5.0f, -18},
            {6, 10},
            snake
        );
    }

    r.pop();

    // Snake hair: five moving snakes
    for (int i = 0; i < 5; i++) {
        const float x = -26.0f + i * 13.0f;

        const float sway =
            std::sin(enemy.walkTime * 4.0f + i * 1.4f)
            * 5.0f;

        r.push();

        r.translate({x, 104});
        r.rotate(sway - 15.0f + i * 7.0f);

        // Snake body
        r.ellipse({0, 2}, {5, 13}, snake);

        // Snake head
        r.ellipse(
            {2, 12}, {8, 5},
            (i % 2 == 0) ? SNAKE_LITE : snake
        );

        // Snake eye
        r.circle({7, 13}, 1.4f, LASER);

        r.pop();
    }

    // Face
    r.ellipse({0, 102}, {17, 16}, green);

    // Eyebrow
    r.rect({10, 109}, {10, 3}, snake);

    // Two red eyes
    r.ellipse({12, 104}, {4.3f, 3.0f}, LASER);
    r.ellipse({14, 98}, {3.6f, 2.6f}, LASER);

    // Eye highlight
    r.circle({13, 104}, 1.7f,
             {1.0f, 0.9f, 0.70f, 1.0f});

    // Mouth
    r.rect({13, 91}, {9, 2}, snake);

    r.pop();
    r.pop();
}

// Draw Medusa's short-range eye lasers
void drawMedusaLaser(
    Renderer& r,
    const Enemy& enemy
) {
    // No laser if she isn't attacking
    if (enemy.laserTime <= 0.0f) return;

    r.push();

    r.translate({enemy.x, enemy.footY()});
    r.scale({-1.0f, 1.0f});

    // Laser fades away after firing
    const float opacity = enemy.laserTime / 0.25f;

    // Each eye shoots one beam
    for (float eyeY : {104.0f, 98.0f}) {
        // Bright eye
        r.circle(
            {18, eyeY}, 6,
            {1.0f, 0.1f, 0.1f, opacity * 0.7f}
        );

        // Laser glow
        r.rect(
            {66, eyeY}, {98, 11},
            {1.0f, 0.08f, 0.12f, opacity * 0.3f}
        );

        // Actual laser beam
        r.rect(
            {66, eyeY}, {98, 3.5f},
            {1.0f, 0.20f, 0.17f, opacity}
        );

        // Laser endpoint
        r.circle(
            {116, eyeY}, 4,
            {1.0f, 0.72f, 0.40f, opacity}
        );
    }

    r.pop();
}
