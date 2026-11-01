#include "Environment.h"
#include "Config.h"
#include <cmath>

// ---- Palette ----
static const glm::vec4 SAND_PALE   (0.91f, 0.79f, 0.54f, 1.0f);
static const glm::vec4 SAND_SHADE  (0.74f, 0.60f, 0.38f, 1.0f);
static const glm::vec4 DUNE_A      (0.88f, 0.70f, 0.44f, 1.0f);
static const glm::vec4 DUNE_B      (0.80f, 0.60f, 0.36f, 1.0f);
static const glm::vec4 GOLD        (0.96f, 0.79f, 0.26f, 1.0f);
static const glm::vec4 GOLD_DARK   (0.68f, 0.50f, 0.12f, 1.0f);
static const glm::vec4 GRANITE     (0.66f, 0.30f, 0.24f, 1.0f);
static const glm::vec4 GRANITE_DK  (0.48f, 0.20f, 0.17f, 1.0f);
static const glm::vec4 PALM_GREEN  (0.20f, 0.52f, 0.25f, 1.0f);
static const glm::vec4 PALM_DARK   (0.12f, 0.38f, 0.19f, 1.0f);
static const glm::vec4 TRUNK       (0.52f, 0.36f, 0.20f, 1.0f);
static const glm::vec4 TRUNK_DARK  (0.34f, 0.22f, 0.11f, 1.0f);
static const glm::vec4 CLAY        (0.74f, 0.40f, 0.22f, 1.0f);
static const glm::vec4 CLAY_DARK   (0.54f, 0.27f, 0.14f, 1.0f);
static const glm::vec4 TURQUOISE   (0.15f, 0.68f, 0.64f, 1.0f);
static const glm::vec4 TOMB_STONE  (0.44f, 0.34f, 0.26f, 1.0f);
static const glm::vec4 TOMB_DARK   (0.22f, 0.16f, 0.12f, 1.0f);

static const float TOP_Y = LANE_COUNT * LANE_H;      // 750 = horizon line / top of the lanes

// -------------------------------------------------------------------------------------
// Backdrop pieces (all drawn with their base on y = baseY)
// -------------------------------------------------------------------------------------
static void drawPyramid(Renderer& r, float x, float baseY, float w, float h) {
    r.triangle({x, baseY + h * 0.5f}, {w, h}, SAND_PALE);                                   // lit face
    r.polygon({{x, baseY}, {x + w * 0.5f, baseY}, {x, baseY + h}}, SAND_SHADE);             // shaded face
    for (int k = 1; k < 5; k++) {                                                           // stone courses
        float t = k / 5.0f;
        r.rect({x, baseY + h * t}, {w * (1.0f - t), 1.5f}, {SAND_SHADE.r, SAND_SHADE.g, SAND_SHADE.b, 0.55f});
    }
}

static void drawObelisk(Renderer& r, float x, float baseY, float s) {
    r.push();
    r.translate({x, baseY});
    r.scale({s, s});
    r.rect({0, 4}, {32, 8}, GRANITE_DK);                                                    // plinth
    r.polygon({{-11, 8}, {11, 8}, {7, 92}, {-7, 92}}, GRANITE);                             // shaft
    r.polygon({{0, 8}, {11, 8}, {7, 92}, {0, 92}}, GRANITE_DK);                             // shaded side
    r.triangle({0, 101}, {14, 18}, GOLD);                                                   // gilded tip
    for (int k = 0; k < 5; k++)                                                             // carved glyph marks
        r.rect({-1, 22 + k * 14.0f}, {5, 3}, {GOLD.r, GOLD.g, GOLD.b, 0.85f});
    r.pop();
}

static void drawPalm(Renderer& r, float x, float baseY, float s, float time) {
    float sway = 1.5f * std::sin(time * 1.2f + x * 0.01f);
    r.push();
    r.translate({x, baseY});
    r.scale({s, s});
    r.rotate(sway);
    r.polygon({{-7, 0}, {7, 0}, {4.5f, 92}, {-4.5f, 92}}, TRUNK);                           // trunk
    for (int k = 1; k < 9; k++)                                                             // trunk rings
        r.rect({0, k * 10.5f}, {12.0f - k * 0.7f, 2}, {TRUNK_DARK.r, TRUNK_DARK.g, TRUNK_DARK.b, 0.7f});

    r.push();
    r.translate({0, 92});                                                                   // crown
    const float angles[3] = {8.0f, 38.0f, 68.0f};
    for (int side = -1; side <= 1; side += 2) {
        for (float a : angles) {
            r.push();
            r.scale({(float)side, 1.0f});
            r.rotate(a);
            r.polygon({{0, 2}, {22, 9}, {52, 10}, {80, -6}, {54, -1}, {24, 1}, {0, -2}}, PALM_GREEN);
            r.polygon({{0, 0}, {26, 2}, {58, -2}, {80, -6}, {54, -1}, {24, 1}}, PALM_DARK);  // shaded underside
            r.pop();
        }
    }
    r.push();
    r.rotate(90.0f);
    r.polygon({{0, 2}, {20, 7}, {44, 6}, {62, -4}, {44, -1}, {20, 1}, {0, -2}}, PALM_GREEN);
    r.pop();
    r.circle({-5, -5}, 3.5f, TRUNK_DARK);                                                   // dates
    r.circle({5, -5}, 3.5f, TRUNK_DARK);
    r.circle({0, -8}, 3.5f, TRUNK_DARK);
    r.pop();
    r.pop();
}

void drawBackdrop(Renderer& r, float time) {
    // sky: warm bands from the horizon up to a pale blue
    const glm::vec4 sky[5] = {
        {0.99f, 0.84f, 0.58f, 1}, {0.98f, 0.80f, 0.60f, 1}, {0.93f, 0.78f, 0.66f, 1},
        {0.80f, 0.78f, 0.76f, 1}, {0.62f, 0.74f, 0.82f, 1}
    };
    float bandH = (VIRTUAL_H - TOP_Y) / 5.0f;
    for (int i = 0; i < 5; i++)
        r.rectBL({0, TOP_Y + i * bandH}, {VIRTUAL_W, bandH + 1.0f}, sky[i]);

    // sun with soft glow rings
    glm::vec2 sun(1290, 835);
    r.circle(sun, 78, {1.0f, 0.90f, 0.55f, 0.18f});
    r.circle(sun, 60, {1.0f, 0.88f, 0.50f, 0.25f});
    r.circle(sun, 42, {1.0f, 0.82f, 0.30f, 1.0f});
    r.circle(sun, 34, {1.0f, 0.92f, 0.55f, 1.0f});

    // far dunes on the horizon (lanes are drawn after this and cover their lower halves)
    r.ellipse({160,  TOP_Y}, {260, 40}, DUNE_B);
    r.ellipse({620,  TOP_Y}, {330, 34}, DUNE_A);
    r.ellipse({1100, TOP_Y}, {300, 42}, DUNE_B);
    r.ellipse({1500, TOP_Y}, {260, 36}, DUNE_A);

    // monuments
    drawPyramid(r, 470, TOP_Y + 4, 190, 108);
    drawPyramid(r, 640, TOP_Y + 4, 120, 70);
    drawPyramid(r, 1080, TOP_Y + 4, 150, 86);
    drawObelisk(r, 860, TOP_Y + 4, 1.0f);
    drawObelisk(r, 1500, TOP_Y + 4, 0.85f);

    // palms
    drawPalm(r, 110, TOP_Y + 4, 0.76f, time);
    drawPalm(r, 340, TOP_Y + 4, 0.66f, time + 1.3f);
    drawPalm(r, 760, TOP_Y + 4, 0.72f, time + 2.1f);
    drawPalm(r, 1420, TOP_Y + 4, 0.66f, time + 0.7f);
}

// -------------------------------------------------------------------------------------
// Field decor
// -------------------------------------------------------------------------------------
static void drawJar(Renderer& r, glm::vec2 base, float s) {
    r.push();
    r.translate(base);
    r.scale({s, s});
    r.ellipse({0, 3}, {10, 3}, {0, 0, 0, 0.18f});                           // ground shadow
    r.rect({0, 3}, {12, 5}, CLAY_DARK);                                     // foot
    r.ellipse({0, 18}, {13, 16}, CLAY);                                     // body
    r.ellipse({4, 18}, {7, 14}, {CLAY_DARK.r, CLAY_DARK.g, CLAY_DARK.b, 0.45f});   // shading
    r.rect({0, 20}, {25, 4}, TURQUOISE);                                    // painted band
    r.rect({0, 14}, {23, 2}, GOLD_DARK);
    r.rect({0, 36}, {8, 9}, CLAY);                                          // neck
    r.ellipse({0, 41}, {9, 3}, CLAY_DARK);                                  // rim
    r.circle({-14, 28}, 3.5f, CLAY_DARK);                                   // handles
    r.circle({14, 28}, 3.5f, CLAY_DARK);
    r.pop();
}

static void drawBrazier(Renderer& r, glm::vec2 base, float time, float phase, float s) {
    float f  = std::sin(time * 11.0f + phase);
    float f2 = std::sin(time * 16.0f + phase * 2.1f);
    r.push();
    r.translate(base);
    r.scale({s, s});
    r.ellipse({0, 2}, {16, 4}, {0, 0, 0, 0.20f});                           // ground shadow
    r.circle({0, 44}, 34.0f + f * 3.0f, {1.0f, 0.55f, 0.10f, 0.14f});       // fire glow
    r.rect({0, 3}, {28, 6}, GOLD_DARK);                                     // base plate
    r.polygon({{-8, 5}, {8, 5}, {5, 28}, {-5, 28}}, GOLD_DARK);             // pedestal
    r.rect({0, 16}, {16, 3}, GOLD);                                         // pedestal ring
    r.polygon({{-18, 34}, {18, 34}, {13, 25}, {6, 21}, {-6, 21}, {-13, 25}}, GOLD_DARK);   // bowl
    r.rect({0, 35}, {40, 4}, GOLD);                                         // rim
    r.triangle({0, 54 + f * 3}, {20 + f2 * 3, 32 + f * 5}, {1.0f, 0.45f, 0.08f, 1});      // flame
    r.triangle({0, 51 + f * 2}, {10 + f2 * 2, 20 + f2 * 3}, {1.0f, 0.88f, 0.30f, 1});     // inner flame
    r.pop();
}

static void drawTombDoors(Renderer& r) {
    const float wallX = VIRTUAL_W - 36.0f;
    r.rectBL({wallX, 0}, {36, TOP_Y}, TOMB_STONE);                          // wall
    r.rectBL({wallX, 0}, {4, TOP_Y}, TOMB_DARK);                            // inner edge shadow
    for (float y = 30.0f; y < TOP_Y; y += 50.0f)                            // stone courses
        r.rect({wallX + 18, y}, {32, 2}, {TOMB_DARK.r, TOMB_DARK.g, TOMB_DARK.b, 0.6f});

    for (int lane = 0; lane < LANE_COUNT; lane++) {
        float cy = laneCenterY(lane) - 8.0f;
        r.rect({wallX + 18, cy},      {30, 96}, TOMB_DARK);                 // frame
        r.rect({wallX + 18, cy - 4},  {20, 84}, {0.03f, 0.02f, 0.03f, 1});  // dark opening
        r.rect({wallX + 18, cy + 52}, {34, 8}, GOLD_DARK);                  // lintel
        r.circle({wallX + 18, cy + 56}, 4.0f, {0.75f, 0.15f, 0.12f, 1});    // red jewel
        r.rect({wallX + 12, cy - 4},  {3, 70}, {0.45f, 0.10f, 0.08f, 0.35f}); // faint red glow inside
    }
}

void drawFieldDecor(Renderer& r, float time) {
    // jars along the bottom edge of lanes   (lane, column)
    const int jars[6][2] = {{0, 3}, {1, 7}, {2, 2}, {3, 6}, {4, 4}, {2, 8}};
    for (auto& j : jars) {
        float x = FIELD_X0 + j[1] * CELL_W + CELL_W * 0.5f + 28.0f;
        drawJar(r, {x, laneBottomY(j[0]) + 8.0f}, 0.85f);
    }

    // brazier stands along the top edge of lanes
    const int braziers[5][2] = {{0, 5}, {1, 1}, {3, 3}, {4, 8}, {2, 6}};
    int n = 0;
    for (auto& b : braziers) {
        float x = FIELD_X0 + b[1] * CELL_W + CELL_W * 0.5f - 30.0f;
        drawBrazier(r, {x, laneBottomY(b[0]) + LANE_H - 56.0f}, time, n++ * 1.9f, 0.7f);
    }

    drawTombDoors(r);
}