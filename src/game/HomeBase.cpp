#include "HomeBase.h"
#include "Config.h"
#include <cmath>
#include <vector>

// ---- Palette: painted lapis stone + gold (must stand out against the sand lanes) ----
static const glm::vec4 STONE      (0.22f, 0.29f, 0.50f, 1.0f);
static const glm::vec4 STONE_DARK (0.13f, 0.17f, 0.31f, 1.0f);
static const glm::vec4 OUTLINE    (0.07f, 0.09f, 0.18f, 1.0f);
static const glm::vec4 GOLD       (0.96f, 0.79f, 0.26f, 1.0f);
static const glm::vec4 GOLD_DARK  (0.68f, 0.50f, 0.12f, 1.0f);
static const glm::vec4 DOOR       (0.05f, 0.04f, 0.07f, 1.0f);
static const glm::vec4 RED        (0.80f, 0.18f, 0.14f, 1.0f);
static const glm::vec4 TURQUOISE  (0.15f, 0.68f, 0.64f, 1.0f);
static const glm::vec4 CREAM      (0.96f, 0.92f, 0.80f, 1.0f);
static const glm::vec4 WOOD       (0.38f, 0.24f, 0.12f, 1.0f);

// pylon walls slope inwards: right edge x goes from 192 (bottom) to 178 (top)
static const float TOP_Y = LANE_COUNT * LANE_H;      // 750

static void drawWingedSun(Renderer& r, glm::vec2 c) {
    r.push();
    r.translate(c);
    for (int side = -1; side <= 1; side += 2) {            // left wing, right wing
        r.push();
        r.scale({(float)side, 1.0f});
        r.polygon({{6, 4}, {44, 3}, {40, -1}, {32, -4}, {22, -6}, {6, -6}}, TURQUOISE);
        r.polygon({{8, 1}, {38, 0}, {30, -2}, {8, -3}}, GOLD);
        r.pop();
    }
    r.circle({0, 0}, 8.0f, GOLD_DARK);
    r.circle({0, 0}, 6.0f, RED);
    r.pop();
}

static void drawTorch(Renderer& r, glm::vec2 base, float time, float phase) {
    float f  = std::sin(time * 12.0f + phase);
    float f2 = std::sin(time * 17.0f + phase * 2.3f);
    r.push();
    r.translate(base);
    r.rect({0, 18}, {6, 40}, WOOD);                         // handle
    r.rect({0, 40}, {14, 8}, GOLD_DARK);                    // cup
    r.triangle({0, 56 + f * 3}, {14 + f2 * 2, 26 + f * 5}, {1.0f, 0.45f, 0.08f, 1});   // outer flame
    r.triangle({0, 53 + f * 2}, {7 + f2, 15 + f2 * 3}, {1.0f, 0.88f, 0.30f, 1});        // inner flame
    r.pop();
}

static void drawAnkh(Renderer& r, glm::vec2 c) {
    r.push();
    r.translate(c);
    r.ellipse({0, 22}, {9, 12}, GOLD);                      // loop
    r.ellipse({0, 22}, {5, 8}, STONE);                      // hole in the loop
    r.rect({0, -10}, {6, 40}, GOLD);                        // stem
    r.rect({0, 6},   {24, 6}, GOLD);                        // crossbar
    r.pop();
}

static void drawEyeOfHorus(Renderer& r, glm::vec2 c) {
    r.push();
    r.translate(c);
    r.ellipse({0, 0}, {14, 8}, CREAM);                      // eye
    r.circle({0, 0}, 5.5f, DOOR);                           // pupil
    r.circle({0, 0}, 2.0f, CREAM);                          // highlight
    r.rect({0, 11}, {28, 3}, GOLD);                         // brow
    r.rect({-5, -14}, {3, 11}, GOLD);                       // tear line
    r.circle({4, -16}, 4.0f, GOLD);                         // spiral
    r.circle({4, -16}, 2.0f, STONE);
    r.pop();
}

static void drawDoorway(Renderer& r, int lane, float time) {
    const float cy = laneCenterY(lane) - 8.0f;   // shifted down so the top lane clears the cornice
    const float dx = 96.0f;

    r.rect({dx, cy - 5}, {84, 116}, GOLD);                  // gold frame
    r.rect({dx, cy + 60}, {94, 16}, GOLD);                  // lintel
    r.rect({dx, cy + 53}, {94, 3}, GOLD_DARK);              // lintel shadow line
    r.rect({dx, cy - 9}, {64, 98}, DOOR);                   // dark opening
    float glow = 0.42f + 0.08f * std::sin(time * 6.0f + lane);
    r.rect({dx, cy - 9}, {44, 78}, {0.55f, 0.25f, 0.06f, glow});   // warm light from inside
    r.rect({dx, cy - 56}, {64, 6}, STONE_DARK);             // threshold step

    drawWingedSun(r, {dx, cy + 60});
    drawTorch(r, {22.0f, cy - 40.0f}, time, lane * 1.7f);

    if (lane % 2 == 0) drawAnkh(r, {154.0f, cy});
    else               drawEyeOfHorus(r, {154.0f, cy + 4.0f});
}

void drawHomeBase(Renderer& r, float time) {
    // soft shadow cast onto the sand to the right of the pylon
    r.rectBL({FIELD_X0,        0}, {8, TOP_Y}, {0, 0, 0, 0.22f});
    r.rectBL({FIELD_X0 + 8.0f, 0}, {8, TOP_Y}, {0, 0, 0, 0.10f});

    // sloped pylon: dark outline first, then the painted stone on top
    r.polygon({{0, 0}, {200, 0}, {186, TOP_Y}, {0, TOP_Y}}, OUTLINE);
    r.polygon({{0, 0}, {192, 0}, {178, TOP_Y}, {0, TOP_Y}}, STONE);

    // stone courses (horizontal joints) and staggered vertical joints
    int row = 0;
    for (float y = 25.0f; y < TOP_Y - 20.0f; y += 50.0f, row++) {
        r.rect({85, y}, {170, 2}, {STONE_DARK.r, STONE_DARK.g, STONE_DARK.b, 0.7f});
        if (y + 50.0f > TOP_Y - 12.0f) continue;   // NEW: top row: no vertical joints above the cornic
        float jx = 55.0f + (row % 2) * 50.0f;
        r.rect({jx,         y + 25}, {2, 48}, {STONE_DARK.r, STONE_DARK.g, STONE_DARK.b, 0.5f});
        r.rect({jx + 90.0f, y + 25}, {2, 48}, {STONE_DARK.r, STONE_DARK.g, STONE_DARK.b, 0.5f});
    }

    // base plinth
    r.rectBL({0, 0}, {196, 12}, STONE_DARK);

    // doorways, torches and hieroglyphs, one set per lane
    for (int lane = 0; lane < LANE_COUNT; lane++)
        drawDoorway(r, lane, time);

    // gold cornice along the top, studded with jewels
    r.rectBL({0, TOP_Y - 12.0f}, {180, 12}, GOLD);
    r.rectBL({0, TOP_Y - 12.0f}, {180, 3}, GOLD_DARK);
    for (int k = 0; k < 6; k++)
        r.circle({16.0f + k * 29.0f, TOP_Y - 6.0f}, 3.5f, (k % 2) ? TURQUOISE : RED);
}