#include "TempleMenus.h"

#include "Config.h"
#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "enemy/Medusa.h"
#include "enemy/Mummy.h"
#include "enemy/Pharaoh.h"

#include "audio/Audio.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
using Color = glm::vec4;
using Glyph = std::array<const char*, 7>;

constexpr float INTRO_SECONDS = 17.0f;
constexpr float INSTRUCTIONS_SECONDS = 3.5f;
const Color GOLD  {0.95f, 0.74f, 0.28f, 1.0f};
const Color IVORY {0.98f, 0.91f, 0.73f, 1.0f};
const Color INK   {0.07f, 0.065f, 0.095f, 1.0f};
const Color DARK  {0.10f, 0.09f, 0.14f, 1.0f};

// Small self-contained 5x7 bitmap font. Every letter is drawn with the existing
// Renderer::rectBL(), so we need no font files, textures, or new OpenGL libraries.
const Glyph& glyph(char c) {
    static const std::unordered_map<char, Glyph> font = {
        {'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
        {'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
        {'C', {" ####", "#    ", "#    ", "#    ", "#    ", "#    ", " ####"}},
        {'D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}},
        {'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
        {'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
        {'G', {" ####", "#    ", "#    ", "#  ##", "#   #", "#   #", " ####"}},
        {'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
        {'I', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"}},
        {'J', {"#####", "    #", "    #", "    #", "#   #", "#   #", " ### "}},
        {'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
        {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
        {'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
        {'N', {"#   #", "##  #", "##  #", "# # #", "#  ##", "#  ##", "#   #"}},
        {'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
        {'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
        {'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
        {'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
        {'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
        {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
        {'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
        {'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
        {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "## ##", "#   #"}},
        {'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
        {'Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}},
        {'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
        {'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
        {'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", "#####"}},
        {'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
        {'3', {"#### ", "    #", "    #", " ### ", "    #", "    #", "#### "}},
        {'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
        {'5', {"#####", "#    ", "#    ", "#### ", "    #", "    #", "#### "}},
        {'6', {" ####", "#    ", "#    ", "#### ", "#   #", "#   #", " ### "}},
        {'7', {"#####", "    #", "   # ", "   # ", "  #  ", "  #  ", "  #  "}},
        {'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
        {'9', {" ### ", "#   #", "#   #", " ####", "    #", "    #", "#### "}},
        {'!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}},
        {'?', {" ### ", "#   #", "    #", "   # ", "  #  ", "     ", "  #  "}},
        {':', {"     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     "}},
        {'/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}},
        {'-', {"     ", "     ", "     ", "#####", "     ", "     ", "     "}},
        {'.', {"     ", "     ", "     ", "     ", "     ", "  ## ", "  ## "}},
        {',', {"     ", "     ", "     ", "     ", "  ## ", "  ## ", "  #  "}},
        {'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
        {'%', {"##  #", "##  #", "   # ", "  #  ", " #   ", "#  ##", "#  ##"}},
        {' ', {"     ", "     ", "     ", "     ", "     ", "     ", "     "}},
    };
    auto it = font.find(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    return it == font.end() ? font.at('?') : it->second;
}

void text(Renderer& r, const std::string& message, float cx, float cy,
          float height, Color color) {
    // Fit very long captions within the 1600px virtual viewport.
    const float fitPixel = (VIRTUAL_W - 100.0f) / std::max(1.0f, message.size() * 6.0f - 1.0f);
    const float pixel = std::min(height / 7.0f, fitPixel);
    height = pixel * 7.0f;
    const float letterW = pixel * 6.0f;
    const float x0 = cx - (message.size() * letterW - pixel) * 0.5f;
    const float y0 = cy - height * 0.5f;

    for (std::size_t i = 0; i < message.size(); ++i) {
        const auto& rows = glyph(message[i]);
        for (int y = 0; y < 7; ++y) {
            // Coalesce adjacent lit pixels into one rectangle per row.
            for (int x = 0; x < 5;) {
                if (rows[y][x] != '#') { ++x; continue; }
                const int begin = x;
                while (x < 5 && rows[y][x] == '#') ++x;
                r.rectBL({x0 + i * letterW + begin * pixel,
                          y0 + (6 - y) * pixel},
                         {(x - begin) * pixel * 0.94f, pixel * 0.94f}, color);
            }
        }
    }
}

bool inside(glm::vec2 p, float cx, float cy, float w, float h) {
    return p.x >= cx - w * 0.5f && p.x <= cx + w * 0.5f &&
           p.y >= cy - h * 0.5f && p.y <= cy + h * 0.5f;
}

bool click(const Game& game, float cy, float w = 390.0f, float h = 70.0f) {
    return game.input.leftClicked() && inside(game.input.mouse, 800.0f, cy, w, h);
}

void borderPanel(Renderer& r, float cx, float cy, float w, float h,
                 Color fill = {0.06f, 0.065f, 0.12f, 0.95f}) {
    r.rect({cx, cy}, {w + 12.0f, h + 12.0f}, {0.34f, 0.23f, 0.09f, 1.0f});
    r.rect({cx, cy}, {w + 6.0f, h + 6.0f}, GOLD);
    r.rect({cx, cy}, {w, h}, fill);
    r.rect({cx, cy + h * 0.5f - 18.0f}, {w - 35.0f, 2.0f}, {0.70f, 0.48f, 0.18f, 1.0f});
    r.rect({cx, cy - h * 0.5f + 18.0f}, {w - 35.0f, 2.0f}, {0.70f, 0.48f, 0.18f, 1.0f});
}

void drawButton(const Game& game, Renderer& r, const std::string& title,
                float cy, float w = 390.0f, float h = 70.0f) {
    const bool hot = inside(game.input.mouse, 800.0f, cy, w, h);
    r.rect({800.0f, cy}, {w + 8.0f, h + 8.0f}, hot ? IVORY : GOLD);
    r.rect({800.0f, cy}, {w, h}, hot ? Color{0.29f, 0.21f, 0.14f, 1.0f} : DARK);
    text(r, title, 800.0f, cy, 32.0f, hot ? Color{1.0f, 0.95f, 0.80f, 1.0f} : IVORY);
}

void cinematicSky(Renderer& r, float t, bool danger) {
    const Color low  = danger ? Color{0.43f, 0.20f, 0.20f, 1} : Color{0.90f, 0.46f, 0.19f, 1};
    const Color high = danger ? Color{0.045f, 0.055f, 0.17f, 1} : Color{0.15f, 0.12f, 0.29f, 1};
    for (int i = 0; i < 18; ++i) {
        const float v = static_cast<float>(i) / 17.0f;
        r.rectBL({0, i * 50.0f}, {VIRTUAL_W, 51.0f}, glm::mix(low, high, v));
    }
    const float pulse = 1.0f + 0.04f * std::sin(t * 2.1f);
    r.circle({1300.0f, 660.0f}, 138 * pulse, {1.0f, 0.53f, 0.22f, 0.10f});
    r.circle({1300.0f, 660.0f}, 94 * pulse, {1.0f, 0.64f, 0.25f, 0.23f});
    r.circle({1300.0f, 660.0f}, 65 * pulse, {1.0f, 0.84f, 0.50f, 0.95f});
    if (danger) {
        for (int i = 0; i < 40; ++i) {
            const float x = std::fmod(i * 341.0f + 49.0f, 1560.0f) + 20.0f;
            const float y = 520.0f + std::fmod(i * 97.0f, 335.0f);
            r.circle({x, y}, (i % 3 + 1) * 1.15f,
                     {1.0f, 0.86f, 0.56f, 0.45f + 0.30f * std::sin(t * 2.0f + i)});
        }
    }
}

void pyramid(Renderer& r, float x, float base, float w, float h, Color a, Color b) {
    r.triangle({x, base + h * 0.5f}, {w, h}, a);
    r.polygon({{x, base + h}, {x + w * 0.5f, base}, {x, base}}, b);
}

void sand(Renderer& r, float t) {
    // Elliptical dunes keep each renderer primitive convex.
    r.rectBL({0, 0}, {VIRTUAL_W, 155}, {0.52f, 0.32f, 0.19f, 1});
    r.ellipse({160, 155}, {390, 85}, {0.52f, 0.32f, 0.19f, 1});
    r.ellipse({690, 149}, {460, 66}, {0.52f, 0.32f, 0.19f, 1});
    r.ellipse({1320, 148}, {440, 74}, {0.52f, 0.32f, 0.19f, 1});
    r.rectBL({0, 0}, {VIRTUAL_W, 90}, {0.76f, 0.49f, 0.25f, 1});
    r.ellipse({300, 88}, {540, 42}, {0.76f, 0.49f, 0.25f, 1});
    r.ellipse({1150, 86}, {660, 39}, {0.76f, 0.49f, 0.25f, 1});
    for (int i = 0; i < 40; ++i) {
        const float x = std::fmod(i * 167.0f + t * (10 + i % 5) * 3, VIRTUAL_W);
        const float y = 115.0f + std::fmod(i * 71.0f + t * 9, 520.0f);
        r.circle({x, y}, static_cast<float>((i % 3) + 1),
                 {1.0f, 0.78f, 0.47f, 0.28f});
    }
}

// Ancient Egyptian temple pylon: two sloped towers, a central gate with a winged sun disk,
// flag masts and the SACRED FLAME burning inside the doorway (what the players defend).
// (x, base) is the bottom centre; s scales the whole building.
void pylon(Renderer& r, float x, float base, float s, float t) {
    const Color stone  {0.83f, 0.69f, 0.47f, 1};
    const Color shade  {0.66f, 0.52f, 0.35f, 1};
    const Color dark   {0.40f, 0.30f, 0.20f, 1};
    const Color relief {0.71f, 0.57f, 0.38f, 1};
    const Color band   {0.90f, 0.74f, 0.40f, 1};
    const Color lapis  {0.16f, 0.32f, 0.66f, 1};
    const Color flagRed{0.78f, 0.16f, 0.12f, 1};

    r.push();
    r.translate({x, base});
    r.scale({s, s});

    for (int side : {-1, 1}) {
        const float sx = static_cast<float>(side);
        // flag masts in front of the towers, pennants waving in the wind
        for (float mx : {105.0f, 205.0f}) {
            r.rect({sx * mx, 190}, {7, 380}, dark);
            const float wave = 6.0f * std::sin(t * 4.0f + mx * 0.05f);
            r.polygon({{sx * mx, 380}, {sx * mx + 46, 372 + wave}, {sx * mx, 356}}, flagRed);
        }
        // sloped tower (convex trapezoid)
        r.polygon({{sx * 60, 0}, {sx * 250, 0}, {sx * 232, 285}, {sx * 72, 285}}, stone);
        r.polygon({{sx * 60, 0}, {sx * 95, 0}, {sx * 104, 285}, {sx * 72, 285}}, shade);
        // cavetto cornice and torus moulding on top
        r.rect({sx * 152, 292}, {176, 16}, band);
        r.rect({sx * 152, 304}, {184, 8}, dark);
        r.rect({sx * 152, 281}, {166, 5}, dark);
        // carved relief: a king offering to a god, plus hieroglyph columns
        for (int k = 0; k < 2; ++k) {
            const float fx = sx * (130.0f + k * 62.0f);
            r.circle({fx, 210}, 11, relief);
            r.rect({fx, 172}, {16, 48}, relief);
            r.rect({fx, 128}, {20, 40}, relief);
            r.rect({fx + sx * 14, 186}, {20, 6}, relief);
        }
        for (int c = 0; c < 4; ++c)
            for (int g = 0; g < 5; ++g)
                r.rect({sx * (115.0f + c * 30.0f), 40.0f + g * 14.0f}, {10, 7}, relief);
    }

    // central gate with a lintel and the winged sun disk
    r.rect({0, 115}, {124, 230}, stone);
    r.rect({0, 232}, {150, 30}, band);
    r.ellipse({-30, 232}, {26, 7}, lapis);
    r.ellipse({30, 232}, {26, 7}, lapis);
    r.circle({0, 232}, 12, {0.92f, 0.36f, 0.14f, 1});

    // doorway with the sacred flame inside
    r.rect({0, 85}, {66, 170}, {0.08f, 0.06f, 0.06f, 1});
    const float flick = 1.0f + 0.12f * std::sin(t * 13.0f) + 0.06f * std::sin(t * 29.0f);
    r.circle({0, 60}, 40 * flick, {1.0f, 0.55f, 0.15f, 0.20f});
    r.rect({0, 14}, {34, 26}, band);                                   // golden brazier
    r.rect({0, 28}, {44, 6}, dark);
    r.ellipse({0, 52}, {13 * flick, 24 * flick}, {1.0f, 0.48f, 0.10f, 1});
    r.ellipse({0, 47}, {8 * flick, 15 * flick}, {1.0f, 0.86f, 0.35f, 1});

    r.pop();
}

void cinematicScene(Renderer& r, float t, bool danger) {
    cinematicSky(r, t, danger);
    pyramid(r, 1180, 165, 415, 315, {0.65f, 0.42f, 0.27f, 1}, {0.40f, 0.28f, 0.24f, 1});
    pyramid(r, 1420, 165, 270, 205, {0.72f, 0.48f, 0.29f, 1}, {0.43f, 0.31f, 0.24f, 1});
    pylon(r, 300, 150, 0.95f, t);
    sand(r, t);
    if (danger)
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.11f, 0.02f, 0.13f, 0.19f});
}

void buttonWithText(const Game& game, Renderer& r, const std::string& title,
                    float cy, float textH) {
    const float w = 390.0f, h = 70.0f;
    const bool hot = inside(game.input.mouse, 800.0f, cy, w, h);
    r.rect({800.0f, cy}, {w + 8.0f, h + 8.0f}, hot ? IVORY : GOLD);
    r.rect({800.0f, cy}, {w, h}, hot ? Color{0.29f, 0.21f, 0.14f, 1.0f} : DARK);
    text(r, title, 800.0f, cy, textH, hot ? Color{1.0f, 0.95f, 0.80f, 1.0f} : IVORY);
}

// Speech bubble above a character's head. a = 0..1 opacity.
void speech(Renderer& r, const std::string& msg, float speakerX, float headY, float a) {
    if (a <= 0.0f) return;
    const float textH = 21.0f;
    const float letterW = textH / 7.0f * 6.0f;
    const float w = msg.size() * letterW + 34.0f;
    const float h = 46.0f;
    const float cy = headY + 62.0f;
    const float cx = std::clamp(speakerX, w * 0.5f + 12.0f, VIRTUAL_W - w * 0.5f - 12.0f);
    const Color edge{0.22f, 0.14f, 0.06f, a};
    const Color fill{0.99f, 0.95f, 0.84f, a};
    const float tipX = std::clamp(speakerX, cx - w * 0.5f + 20.0f, cx + w * 0.5f - 20.0f);
    r.polygon({{tipX - 13, cy - h * 0.5f + 2}, {tipX + 13, cy - h * 0.5f + 2}, {speakerX, headY + 8}}, edge);
    r.rect({cx, cy}, {w + 6, h + 6}, edge);
    r.rect({cx, cy}, {w, h}, fill);
    r.polygon({{tipX - 9, cy - h * 0.5f + 1}, {tipX + 9, cy - h * 0.5f + 1}, {speakerX, headY + 13}}, fill);
    text(r, msg, cx, cy, textH, {0.18f, 0.10f, 0.05f, a});
}

// opacity for something visible between t0 and t1 with short fades
float window(float t, float t0, float t1) {
    if (t < t0 || t > t1) return 0.0f;
    return std::min({1.0f, (t - t0) / 0.15f, (t1 - t) / 0.15f});
}

void enemyBar(Renderer& r, const Enemy& e) {
    const float width = 70.0f, height = 8.0f, gap = 2.0f;
    const float bottom = e.footY() + 130.0f;
    const float first = e.x - width * 0.5f;
    const float segW = (width - (e.maxHealth - 1) * gap) / e.maxHealth;
    r.rectBL({first - 3, bottom - 3}, {width + 6, height + 6}, {0.12f, 0.10f, 0.11f, 0.92f});
    for (int i = 0; i < e.maxHealth; ++i)
        r.rectBL({first + i * (segW + gap), bottom}, {segW, height},
                 i < e.health ? Color{1.0f, 0.55f, 0.25f, 1} : Color{0.28f, 0.24f, 0.22f, 1});
}

// ---- intro timeline (seconds) ----
constexpr int   CUT_LANE   = 3;
constexpr float PLAYER_H   = 125.0f;
constexpr float ENEMY_IN   = 4.0f;    // the dead arrive
constexpr float BATTLE     = 9.0f;    // the guardians attack
constexpr float TORNADO_AT = 9.3f;    // Mummy's low tornado -> the guard jumps over it
constexpr float DUCK_AT    = 11.5f;
constexpr float BEAM_AT    = 11.7f;   // Medusa's high beam -> the guard ducks under it
constexpr float BOLT_AT    = 13.2f;   // Pharaoh's lightning hits the guard
constexpr float OUTRO      = 14.8f;

float cutGround() { return laneBottomY(CUT_LANE) + 8.0f; }   // same feet height as enemies

float pulse(float t, float at) { return std::clamp(1.0f - std::fabs(t - at) / 0.35f, 0.0f, 1.0f); }

} // namespace

// ------------------------------------------------------------------ intro cast
struct TempleMenus::Cutscene {
    MeleePlayer     melee  {{600.0f, cutGround()}, PLAYER_H};
    LongRangePlayer archer {{430.0f, cutGround()}, PLAYER_H};
    DefensePlayer   shield {{515.0f, cutGround()}, PLAYER_H};
    Enemy mummy  {EnemyKind::Mummy,   CUT_LANE, 1700.0f, 5, 5, 1, 0.0f};
    Enemy medusa {EnemyKind::Medusa,  CUT_LANE, 1830.0f, 5, 5, 1, 0.0f};
    Enemy pharaoh{EnemyKind::Pharaoh, CUT_LANE, 1960.0f, 7, 7, 2, 0.0f};

    bool mummyAlive = true;
    bool coinActive = false;  float coinX = 0.0f;
    bool tornadoFired = false, tornadoActive = false, jumped = false;
    EnemyProjectile tornado{EnemyKind::Mummy, CUT_LANE, 0.0f, 0.0f, 1, 0.0f};
    bool ducked = false, beamFired = false;
    bool boltFired = false;
    EnemyProjectile bolt{EnemyKind::Pharaoh, CUT_LANE, 0.0f, 0.0f, 0, 99.0f};
    float t = 0.0f;

    Cutscene() {
        melee.setSpeed(190.0f);
        archer.setArrowMaxX(VIRTUAL_W);
    }
};

TempleMenus::TempleMenus() = default;
TempleMenus::~TempleMenus() = default;

void TempleMenus::setup(Game& game) {
    game.onAlwaysUpdate([this, &game](float dt) { update(game, dt); });
    game.onDraw(Layer::Menus, [this, &game](Renderer& r) { draw(game, r); });
}

void TempleMenus::updateCutscene(float dt) {
    if (!cutscene_) return;
    Cutscene& c = *cutscene_;
    c.t += dt;
    const float t = c.t;

    c.shield.update(dt);

    // The dead walk in from the right and take their positions.
    auto walkTo = [&](Enemy& e, float target) {
        if (t < ENEMY_IN || e.x <= target) return;
        e.x = std::max(target, e.x - 160.0f * dt);
        e.walkTime += dt;
    };
    if (c.mummyAlive) walkTo(c.mummy, 1020.0f);
    walkTo(c.medusa, 1210.0f);
    walkTo(c.pharaoh, 1400.0f);

    // Attack poses (arms raise just before each attack; the Pharaoh gestures while he talks).
    c.mummy.attackPose   = pulse(t, TORNADO_AT);
    c.medusa.attackPose  = pulse(t, BEAM_AT);
    c.medusa.laserTime   = (t >= BEAM_AT && t < BEAM_AT + 0.25f) ? BEAM_AT + 0.25f - t : 0.0f;
    c.pharaoh.attackPose = std::max(pulse(t, BOLT_AT), (t > 6.6f && t < 8.4f) ? 0.55f : 0.0f);
    c.pharaoh.castFlash  = (t >= BOLT_AT && t < BOLT_AT + 0.25f) ? 0.25f : 0.0f;

    if (t < BATTLE) return;

    // ---- battle ----
    auto hitMummy = [&]() {
        if (!c.mummyAlive) return;
        c.mummy.health = std::max(0, c.mummy.health - 1);
        if (c.mummy.health == 0) {
            c.mummyAlive = false;
            c.coinActive = true;
            c.coinX = c.mummy.x;
            audio::play(audio::Sfx::EnemyDeath);
        } else {
            audio::play(audio::Sfx::EnemyHit);
        }
    };
    auto hitMedusa = [&]() {
        c.medusa.health = std::max(2, c.medusa.health - 1);
        audio::play(audio::Sfx::EnemyHit);
    };

    // Melee guard charges the Mummy, stops when they meet and duels it.
    // Once it falls he steps forward onto the dropped coin and holds the line.
    const float meleeX = c.melee.position().x;
    if (c.mummyAlive)
        c.melee.setEngaged(c.mummy.x - meleeX <= c.mummy.halfWidth() + c.melee.halfWidth() + 2.0f);
    else
        c.melee.setEngaged(meleeX >= c.coinX);
    c.melee.update(dt);
    if (c.mummyAlive &&
        c.melee.attackHits(c.mummy.x - c.mummy.halfWidth(), c.mummy.x + c.mummy.halfWidth()))
        hitMummy();

    // Archer holds her ground behind the shield bearer and shoots at Medusa.
    c.archer.setEngaged(true);
    c.archer.update(dt);
    for (std::size_t i = 0; i < c.archer.arrows().size(); ++i)
        if (c.archer.arrowHitsEnemy(i, c.medusa.x - 23.0f, c.medusa.x + 23.0f)) hitMedusa();

    // The fallen mummy drops a golden ankh coin; the guard picks it up.
    if (c.coinActive && std::fabs(c.melee.position().x - c.coinX) < c.melee.halfWidth() + 19.0f) {
        c.coinActive = false;
        audio::play(audio::Sfx::Coin);
    }

    // Low tornado: the guard JUMPS over it, the shield bearer absorbs it.
    if (!c.tornadoFired && t >= TORNADO_AT && c.mummyAlive) {
        c.tornadoFired = c.tornadoActive = true;
        c.tornado.x = c.mummy.x - 25.0f;
        c.tornado.y = cutGround() + 25.0f;
        c.tornado.age = 0.0f;
        audio::play(audio::Sfx::Tornado);
    }
    if (c.tornadoActive) {
        c.tornado.x -= 260.0f * dt;
        c.tornado.age += dt;
        const float gap = c.tornado.x - c.melee.position().x;
        if (!c.jumped && gap > 0.0f && gap < 75.0f) { c.melee.jump(); c.jumped = true; }
        if (c.tornado.x <= c.shield.position().x + 30.0f) {
            c.tornadoActive = false;
            c.shield.takeHit(1);
            audio::play(audio::Sfx::PlayerHurt);
        }
    }

    // High beam: the guard DUCKS under it.
    if (!c.ducked && t >= DUCK_AT) { c.melee.duck(); c.ducked = true; }
    if (!c.beamFired && t >= BEAM_AT) { c.beamFired = true; audio::play(audio::Sfx::Beam); }

    // Pharaoh's lightning strikes the guard (damage tint + health bar drop).
    if (!c.boltFired && t >= BOLT_AT) {
        c.boltFired = true;
        c.bolt.x = c.melee.position().x;
        c.bolt.y = cutGround() + 60.0f;
        c.bolt.age = 0.0f;
        c.melee.takeHit(2);
        audio::play(audio::Sfx::Thunder);
    }
    c.bolt.age += dt;
}

void TempleMenus::update(Game& game, float dt) {
    if (!observed_ || game.state() != observedState_) {
        observed_ = true;
        observedState_ = game.state();
        stateTime_ = 0.0f;
        if (game.state() == GameState::Cutscene) cutscene_ = std::make_unique<Cutscene>();
        if (game.state() == GameState::GameOver)
            audio::play(game.playerWon() ? audio::Sfx::Win : audio::Sfx::Lose);
    } else {
        stateTime_ += dt;
    }

    if (game.input.keyJustPressed(GLFW_KEY_M)) audio::toggleMute();

    auto pressed = [&](float cy) {
        const bool hit = click(game, cy);
        if (hit) audio::play(audio::Sfx::Click);
        return hit;
    };

    switch (game.state()) {
        case GameState::Cutscene:
            updateCutscene(dt);
            if (game.input.keyJustPressed(GLFW_KEY_ENTER) || stateTime_ >= INTRO_SECONDS) {
                cutscene_.reset();
                game.finishCutscene();                         // -> instructions -> gameplay
            }
            break;

        case GameState::MainMenu:
            if (soundMenu_) {
                if (pressed(445.0f))      audio::volumeUp();
                else if (pressed(365.0f)) audio::volumeDown();
                else if (pressed(285.0f)) audio::toggleMute();
                else if (pressed(205.0f) || game.input.keyJustPressed(GLFW_KEY_ESCAPE))
                    soundMenu_ = false;
            } else {
                if (pressed(460.0f))      game.startGame();    // Start -> cutscene
                else if (pressed(365.0f)) soundMenu_ = true;
                else if (pressed(270.0f)) game.requestExit();
            }
            break;

        case GameState::Instructions:
            if (stateTime_ >= INSTRUCTIONS_SECONDS)
                game.completeInstructions();
            break;

        case GameState::Paused:
            if (pressed(555.0f))      game.resume();
            else if (pressed(460.0f)) game.restart();
            else if (pressed(365.0f)) game.goToMainMenu();
            else if (pressed(270.0f)) game.requestExit();
            break;

        case GameState::GameOver:
            if (pressed(510.0f))      game.restart();
            else if (pressed(410.0f)) game.goToMainMenu();
            else if (pressed(310.0f)) game.requestExit();
            break;

        case GameState::Playing:
            break;
    }
}

void TempleMenus::draw(const Game& game, Renderer& r) const {
    switch (game.state()) {
        case GameState::Cutscene:     drawCutscene(r); break;
        case GameState::MainMenu:     drawMainMenu(game, r); break;
        case GameState::Instructions: drawInstructions(r); break;
        case GameState::Paused:       drawPause(game, r); break;
        case GameState::GameOver:     drawGameOver(game, r); break;
        case GameState::Playing:      break;
    }
}

void TempleMenus::drawCutscene(Renderer& r) const {
    if (!cutscene_) return;
    const Cutscene& c = *cutscene_;
    const float t = std::min(c.t, INTRO_SECONDS);
    const float ground = cutGround();
    const float headY = ground + PLAYER_H * 1.12f;

    // ---- setting: the temple of Ra at dusk; the sky darkens when the dead arrive ----
    cinematicSky(r, t, t >= ENEMY_IN);
    pyramid(r, 980, 165, 415, 315, {0.65f, 0.42f, 0.27f, 1}, {0.40f, 0.28f, 0.24f, 1});
    pyramid(r, 1240, 165, 270, 205, {0.72f, 0.48f, 0.29f, 1}, {0.43f, 0.31f, 0.24f, 1});
    pylon(r, 205, 150, 0.7f, t);
    sand(r, t);
    if (t >= ENEMY_IN)
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H},
                 {0.11f, 0.02f, 0.13f, 0.19f * std::min(1.0f, (t - ENEMY_IN) / 1.0f)});

    // ---- cast ----
    c.archer.draw(r);
    c.shield.draw(r);
    c.melee.draw(r);
    if (c.mummyAlive) drawMummy(r, c.mummy);
    drawMedusa(r, c.medusa);
    drawPharaoh(r, c.pharaoh);

    if (c.coinActive) {
        const float y = ground + 14.0f + 3.0f * std::sin(t * 6.0f);
        r.circle({c.coinX, y}, 16, {0.75f, 0.45f, 0.08f, 1});
        r.circle({c.coinX, y}, 12, {1.0f, 0.83f, 0.27f, 1});
        r.ellipse({c.coinX, y + 4}, {3.5f, 4.5f}, {0.52f, 0.29f, 0.06f, 1});
        r.rect({c.coinX, y - 4}, {2.5f, 10}, {0.52f, 0.29f, 0.06f, 1});
    }

    // ---- attacks, arrows, health bars ----
    c.archer.drawEffects(r);
    c.shield.drawEffects(r);
    c.melee.drawEffects(r);
    if (c.tornadoActive) drawMummyTornado(r, c.tornado);
    drawMedusaLaser(r, c.medusa);
    if (c.bolt.age < 0.3f) drawPharaohLightning(r, c.bolt);
    if (t >= ENEMY_IN + 0.5f) {
        if (c.mummyAlive) enemyBar(r, c.mummy);
        enemyBar(r, c.medusa);
        enemyBar(r, c.pharaoh);
    }

    // ---- dialogue between the guardians and the dead ----
    const float enemyHead = ground + 140.0f;
    speech(r, "THE SACRED FLAME MUST NEVER FALL.", c.shield.position().x, headY, window(t, 0.8f, 3.9f));
    speech(r, "GIVE US THE FLAME, MORTALS!", c.pharaoh.x, enemyHead, window(t, 6.6f, 8.4f));
    speech(r, "NEVER! GUARDIANS, TO ARMS!", c.melee.position().x, headY, window(t, 8.2f, 9.8f));
    speech(r, "JUMP!", c.archer.position().x, headY, window(t, 9.4f, 10.4f));
    speech(r, "DUCK!", c.archer.position().x, headY, window(t, 11.0f, 12.0f));
    speech(r, "WE ARE ENDLESS. WAVE AFTER WAVE!", c.pharaoh.x, enemyHead, window(t, 13.4f, 15.6f));

    // ---- narration captions (support the animation, never replace it) ----
    const float a1 = window(t, 0.3f, 4.0f), a2 = window(t, 4.2f, 8.8f), a3 = window(t, OUTRO, INTRO_SECONDS);
    if (a1 > 0) {
        text(r, "THE TEMPLE OF RA GUARDS THE SACRED FLAME", 800, 790, 40, {IVORY.r, IVORY.g, IVORY.b, a1});
        text(r, "WHILE IT BURNS, THE DEAD OF EGYPT SLEEP", 800, 735, 27, {GOLD.r, GOLD.g, GOLD.b, a1});
    }
    if (a2 > 0) {
        text(r, "BUT TONIGHT THE DEAD HAVE RISEN", 800, 790, 42, {IVORY.r, IVORY.g, IVORY.b, a2});
        text(r, "THEY MARCH ON THE TEMPLE TO PUT OUT THE FLAME", 800, 735, 27, {GOLD.r, GOLD.g, GOLD.b, a2});
    }
    if (a3 > 0) {
        text(r, "DEFEND THE TEMPLE OF RA", 800, 790, 52, {IVORY.r, IVORY.g, IVORY.b, a3});
        text(r, "HOLD THE GATE AGAINST THREE WAVES OF THE DEAD", 800, 730, 27, {GOLD.r, GOLD.g, GOLD.b, a3});
    }

    // fade in at the start, fade to black at the end
    float fade = 0.0f;
    if (t < 0.6f) fade = 1.0f - t / 0.6f;
    if (t > INTRO_SECONDS - 0.6f) fade = (t - (INTRO_SECONDS - 0.6f)) / 0.6f;
    if (fade > 0.0f)
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.01f, 0.01f, 0.03f, std::min(fade, 1.0f)});

    text(r, "PRESS ENTER TO SKIP", 800, 70, 24, IVORY);
    text(r, "M : MUTE / UNMUTE", 800, 38, 17, GOLD);
    r.rectBL({420, 14}, {760, 6}, {0.32f, 0.23f, 0.16f, 0.8f});
    r.rectBL({420, 14}, {760 * t / INTRO_SECONDS, 6}, GOLD);
}

void TempleMenus::drawMainMenu(const Game& game, Renderer& r) const {
    cinematicScene(r, stateTime_, false);
    r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.025f, 0.026f, 0.055f, 0.61f});
    text(r, "ANCIENT EGYPTIAN TEMPLE", 800, 760, 69, IVORY);
    text(r, "DEFEND THE SACRED FLAME OF RA", 800, 680, 34, GOLD);

    if (soundMenu_) { drawSoundSettings(game, r); return; }

    borderPanel(r, 800, 363, 474, 334);
    drawButton(game, r, "START", 460);
    buttonWithText(game, r, "SOUND SETTINGS", 365, 26);
    drawButton(game, r, "EXIT", 270);
    text(r, "M : MUTE / UNMUTE", 800, 88, 23, IVORY);
}

void TempleMenus::drawSoundSettings(const Game& game, Renderer& r) const {
    borderPanel(r, 800, 380, 560, 520);
    text(r, "SOUND SETTINGS", 800, 600, 40, GOLD);

    // volume meter: 10 blocks
    const bool muted = audio::muted();
    const int level = static_cast<int>(std::lround(audio::volume() * 10.0f));
    for (int i = 0; i < 10; ++i) {
        const Color on = muted ? Color{0.45f, 0.42f, 0.40f, 1} : GOLD;
        r.rect({800.0f - 4.5f * 38.0f + i * 38.0f, 545}, {30, 26},
               i < level ? on : Color{0.22f, 0.19f, 0.17f, 1});
    }
    const std::string label = muted ? "MUTED" : "VOLUME " + std::to_string(level * 10) + "%";
    text(r, label, 800, 503, 26, muted ? Color{1.0f, 0.57f, 0.47f, 1.0f} : IVORY);

    drawButton(game, r, "VOLUME +", 445);
    drawButton(game, r, "VOLUME -", 365);
    drawButton(game, r, muted ? "UNMUTE" : "MUTE", 285);
    drawButton(game, r, "BACK", 205);
}

void TempleMenus::drawInstructions(Renderer& r) const {
    // The game board is visible underneath, but no gameplay updates take place yet.
    r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.025f, 0.03f, 0.07f, 0.63f});
    borderPanel(r, 800, 453, 1080, 480);
    text(r, "BEFORE THE BATTLE", 800, 622, 58, GOLD);
    text(r, "DUCK: PRESS S OR DOWN ARROW", 800, 548, 37, IVORY);
    text(r, "JUMP: PRESS W OR UP ARROW", 800, 494, 37, IVORY);
    text(r, "CLICK A LANE, THEN A CARD TO PLACE A WARRIOR", 800, 440, 22, GOLD);
    text(r, "SHIELD: LANE, CARD, THEN CLICK A SPOT IN THE LANE", 800, 400, 22, GOLD);
    text(r, "ESC: PAUSE THE GAME", 800, 352, 27, IVORY);
    r.rectBL({500, 256}, {600, 11}, {0.25f, 0.18f, 0.11f, 1});
    const float remain = std::max(0.0f, 1.0f - stateTime_ / INSTRUCTIONS_SECONDS);
    r.rectBL({500, 256}, {600 * remain, 11}, GOLD);
    text(r, "BATTLE BEGINS IN " + std::to_string(std::max(1, static_cast<int>(std::ceil(INSTRUCTIONS_SECONDS - stateTime_)))),
         800, 300, 25, IVORY);
}

void TempleMenus::drawPause(const Game& game, Renderer& r) const {
    borderPanel(r, 800, 451, 565, 586);
    text(r, "BATTLE PAUSED", 800, 663, 46, GOLD);
    drawButton(game, r, "RESUME", 555);
    drawButton(game, r, "RESTART", 460);
    drawButton(game, r, "MAIN MENU", 365);
    drawButton(game, r, "EXIT", 270);
    text(r, "ESC TO RESUME", 800, 167, 23, IVORY);
}

void TempleMenus::drawGameOver(const Game& game, Renderer& r) const {
    const bool won = game.playerWon();
    borderPanel(r, 800, 450, 620, 565);
    text(r, won ? "VICTORY!" : "THE TEMPLE HAS FALLEN", 800, 655,
         won ? 65.0f : 32.0f, won ? GOLD : Color{1.0f, 0.57f, 0.47f, 1.0f});
    text(r, won ? "THE SACRED FLAME STILL BURNS" : "GATHER YOUR WARRIORS AGAIN",
         800, 586, 23, IVORY);
    drawButton(game, r, "PLAY AGAIN", 510);
    drawButton(game, r, "MAIN MENU", 410);
    drawButton(game, r, "EXIT", 310);
}
