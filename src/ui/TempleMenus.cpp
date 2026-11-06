#include "TempleMenus.h"

#include "Config.h"
#include "DefensePlayer.h"
#include "LongRangePlayer.h"
#include "MeleePlayer.h"
#include "enemy/Medusa.h"
#include "enemy/Mummy.h"
#include "enemy/Pharaoh.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <mmsystem.h>
#endif

namespace {
using Color = glm::vec4;
using Glyph = std::array<const char*, 7>;

constexpr float INTRO_SECONDS = 15.0f;
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

void temple(Renderer& r, float x, float y, float scale) {
    r.push(); r.translate({x, y}); r.scale({scale, scale});
    const Color stone{0.63f, 0.49f, 0.33f, 1};
    const Color shade{0.40f, 0.30f, 0.24f, 1};
    r.rect({0, 34}, {660, 68}, shade);                             // podium
    r.rect({0, 68}, {720, 20}, GOLD);                             // floor
    r.rect({0, 400}, {690, 55}, stone);                          // roof
    r.rect({0, 438}, {740, 17}, GOLD);                           // sunlit cornice
    r.rect({0, 480}, {580, 50}, stone);                          // upper structure
    r.triangle({0, 545}, {610, 92}, GOLD);                       // pediment
    r.triangle({0, 529}, {520, 70}, shade);
    for (int i = -3; i <= 3; ++i) {
        const float colX = i * 95.0f;
        r.rect({colX, 253}, {55, 300}, stone);
        r.rect({colX, 118}, {70, 20}, shade);
        r.rect({colX, 387}, {82, 21}, GOLD);
        r.rect({colX, 366}, {68, 10}, shade);
        for (int j = 0; j < 4; ++j)
            r.rect({colX, 175.0f + j * 43.0f}, {12, 10}, shade);
    }
    r.rect({0, 225}, {115, 300}, {0.07f, 0.065f, 0.08f, 1});       // temple entrance
    r.rect({0, 232}, {82, 276}, {0.16f, 0.12f, 0.10f, 1});
    r.circle({0, 500}, 24, GOLD);                                 // sun-disk glyph
    r.circle({0, 500}, 13, shade);
    r.pop();
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

void cinematicScene(Renderer& r, float t, bool danger, bool closeTemple) {
    cinematicSky(r, t, danger);
    pyramid(r, 250, 165, 415, 315, {0.65f, 0.42f, 0.27f, 1}, {0.40f, 0.28f, 0.24f, 1});
    pyramid(r, 480, 165, 270, 205, {0.72f, 0.48f, 0.29f, 1}, {0.43f, 0.31f, 0.24f, 1});
    r.push();
    r.translate({12.0f * std::sin(t * 0.65f), 3.0f * std::cos(t * 0.45f)});
    temple(r, closeTemple ? 850.0f : 760.0f, 115.0f, closeTemple ? 0.88f : 0.72f);
    r.pop();
    sand(r, t);
    if (danger)
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.11f, 0.02f, 0.13f, 0.19f});
}

std::string locateAudio() {
    namespace fs = std::filesystem;
    std::vector<fs::path> choices = {
        "assets/audio/temple_suspense.wav",
        "../assets/audio/temple_suspense.wav",
        "../../assets/audio/temple_suspense.wav"
    };
#ifdef _WIN32
    char exe[MAX_PATH] = {};
    const DWORD count = GetModuleFileNameA(nullptr, exe, MAX_PATH);
    if (count > 0 && count < MAX_PATH)
        choices.push_back(fs::path(exe).parent_path() / "assets/audio/temple_suspense.wav");
#endif
    for (const auto& path : choices) {
        std::error_code error;
        if (fs::exists(path, error)) return fs::absolute(path, error).string();
    }
    return {};
}

} // namespace

TempleMenus::~TempleMenus() { stopMusic(); }

void TempleMenus::setup(Game& game) {
    audioFile_ = locateAudio();
    game.onAlwaysUpdate([this, &game](float dt) { update(game, dt); });
    game.onDraw(Layer::Menus, [this, &game](Renderer& r) { draw(game, r); });
}

void TempleMenus::stopMusic() {
#ifdef _WIN32
    if (musicPlaying_) PlaySoundA(nullptr, nullptr, 0);
#endif
    musicPlaying_ = false;
}

void TempleMenus::syncMusic(GameState state) {
    const bool wantsMusic = musicEnabled_ &&
        (state == GameState::Cutscene || state == GameState::MainMenu);
    if (!wantsMusic) { stopMusic(); return; }
    if (musicPlaying_) return;
#ifdef _WIN32
    if (!audioFile_.empty()) {
        musicPlaying_ = PlaySoundA(audioFile_.c_str(), nullptr,
                       SND_FILENAME | SND_ASYNC | SND_LOOP | SND_NODEFAULT) != FALSE;
        if (!musicPlaying_) std::cerr << "Intro WAV could not be played.\n";
    }
#endif
}

void TempleMenus::update(Game& game, float dt) {
    if (!observed_ || game.state() != observedState_) {
        observed_ = true;
        observedState_ = game.state();
        stateTime_ = 0.0f;
    } else {
        stateTime_ += dt;
    }

    if (game.input.keyJustPressed(GLFW_KEY_M)) musicEnabled_ = !musicEnabled_;

    switch (game.state()) {
        case GameState::Cutscene:
            if (game.input.keyJustPressed(GLFW_KEY_ENTER) ||
                stateTime_ >= INTRO_SECONDS)
                game.finishCutscene(); // IMPORTANT: goes to main menu, not gameplay.
            break;

        case GameState::MainMenu:
            if (click(game, 460.0f)) game.startGame();         // Start -> instructions
            else if (click(game, 365.0f)) game.replayCutscene();
            else if (click(game, 270.0f)) game.requestExit();
            break;

        case GameState::Instructions:
            if (stateTime_ >= INSTRUCTIONS_SECONDS)
                game.completeInstructions();                  // Instructions -> gameplay
            break;

        case GameState::Paused:
            if (click(game, 555.0f)) game.resume();
            else if (click(game, 460.0f)) game.restart();
            else if (click(game, 365.0f)) game.goToMainMenu();
            else if (click(game, 270.0f)) game.requestExit();
            break;

        case GameState::GameOver:
            if (click(game, 510.0f)) game.restart();
            else if (click(game, 410.0f)) game.goToMainMenu();
            else if (click(game, 310.0f)) game.requestExit();
            break;

        case GameState::Playing:
            break;
    }

    syncMusic(game.state());
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
    const float t = std::min(stateTime_, INTRO_SECONDS);
    const int scene = t < 5.0f ? 0 : (t < 10.0f ? 1 : 2);
    const float local = std::fmod(t, 5.0f);
    cinematicScene(r, t, scene == 1, scene != 0);

    if (scene == 0) {
        // Temple approaches slowly; torchlight pulses in the doorway.
        r.circle({836, 302}, 68 + 8 * std::sin(t * 3.0f),
                 {1.0f, 0.62f, 0.20f, 0.12f});
        text(r, "FOR CENTURIES, THE TEMPLE STOOD PROTECTED",
             800, 774, 43, IVORY);
        text(r, "AN ANCIENT POWER SLEEPS BEHIND ITS GATES",
             800, 703, 28, GOLD);
    } else if (scene == 1) {
        const float approach = std::min(local * 47.0f, 210.0f);
        Enemy medusa{EnemyKind::Medusa, 3, 1160.0f - approach, 5, 5, 1, 55};
        Enemy mummy{EnemyKind::Mummy, 3, 1350.0f - approach * 0.90f, 5, 5, 1, 65};
        Enemy pharaoh{EnemyKind::Pharaoh, 3, 1550.0f - approach * 1.30f, 7, 7, 2, 100};
        medusa.walkTime = mummy.walkTime = pharaoh.walkTime = local * 2.3f;
        medusa.attackPose = 0.6f;
        mummy.attackPose = 0.5f;
        pharaoh.attackPose = 0.75f;
        r.push();
        r.translate({0.0f, 53.0f});
        r.scale({1.0f, 1.32f});
        drawMedusa(r, medusa);
        drawMummy(r, mummy);
        drawPharaoh(r, pharaoh);
        r.pop();
        if (local > 2.7f) {
            const float blink = 0.30f + 0.25f * std::sin(t * 15.0f);
            r.rect({910.0f, 342.0f}, {270.0f, 5.0f}, {1.0f, 0.05f, 0.10f, blink});
        }
        text(r, "BUT DARKNESS HAS AWAKENED AN ARMY",
             800, 774, 45, IVORY);
        text(r, "MEDUSA. MUMMY. PHARAOH.", 800, 704, 34, GOLD);
    } else {
        // Show the actual finished player models without altering their code.
        MeleePlayer melee({320.0f, 175.0f}, 220.0f);
        DefensePlayer shield({550.0f, 175.0f}, 220.0f);
        LongRangePlayer archer({790.0f, 175.0f}, 220.0f);
        r.push();
        r.translate({std::min(local * 17.0f, 55.0f), 4.0f * std::sin(t * 2.0f)});
        melee.draw(r);
        shield.draw(r);
        archer.draw(r);
        r.pop();
        text(r, "THE TEMPLE CALLS FOR ITS GUARDIANS",
             800, 779, 46, IVORY);
        text(r, "COMMAND YOUR WARRIORS. DEFEND THE TEMPLE.",
             800, 698, 32, GOLD);
    }

    // Soft cinematic fade-in and transition between the 3 acts.
    float fade = 0.0f;
    if (t < 0.6f) fade = 1.0f - t / 0.6f;
    if (scene != 2 && local > 4.58f) fade = std::max(fade, (local - 4.58f) / 0.42f);
    if (scene != 0 && local < 0.30f) fade = std::max(fade, 1.0f - local / 0.30f);
    if (fade > 0.0f)
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.01f, 0.01f, 0.03f, std::min(fade, 1.0f)});

    text(r, "PRESS ENTER TO SKIP", 800, 91, 29, IVORY);
    text(r, "M : TOGGLE SOUND", 800, 45, 19, GOLD);
    r.rectBL({420, 17}, {760, 7}, {0.32f, 0.23f, 0.16f, 0.8f});
    r.rectBL({420, 17}, {760 * t / INTRO_SECONDS, 7}, GOLD);
}

void TempleMenus::drawMainMenu(const Game& game, Renderer& r) const {
    cinematicScene(r, stateTime_, false, false);
    r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.025f, 0.026f, 0.055f, 0.61f});
    text(r, "ANCIENT EGYPTIAN TEMPLE", 800, 733, 69, IVORY);
    text(r, "DEFEND THE SACRED GATE", 800, 645, 34, GOLD);
    borderPanel(r, 800, 363, 474, 334);
    drawButton(game, r, "START", 460);
    drawButton(game, r, "REPLAY INTRO", 365);
    drawButton(game, r, "EXIT", 270);
    text(r, "M : TOGGLE SOUND", 800, 88, 23, IVORY);
}

void TempleMenus::drawInstructions(Renderer& r) const {
    // The game board is visible underneath, but no gameplay updates take place yet.
    r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.025f, 0.03f, 0.07f, 0.63f});
    borderPanel(r, 800, 453, 1080, 480);
    text(r, "BEFORE THE BATTLE", 800, 615, 58, GOLD);
    text(r, "DUCK: PRESS S OR DOWN ARROW", 800, 528, 39, IVORY);
    text(r, "JUMP: PRESS W OR UP ARROW", 800, 458, 39, IVORY);
    text(r, "CLICK A LANE TO SELECT IT", 800, 383, 30, GOLD);
    text(r, "ESC: PAUSE THE GAME", 800, 331, 29, IVORY);
    r.rectBL({500, 251}, {600, 11}, {0.25f, 0.18f, 0.11f, 1});
    const float remain = std::max(0.0f, 1.0f - stateTime_ / INSTRUCTIONS_SECONDS);
    r.rectBL({500, 251}, {600 * remain, 11}, GOLD);
    text(r, "BATTLE BEGINS IN " + std::to_string(std::max(1, static_cast<int>(std::ceil(INSTRUCTIONS_SECONDS - stateTime_)))),
         800, 291, 25, IVORY);
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
    text(r, won ? "THE SACRED GATE IS SAFE" : "GATHER YOUR WARRIORS AGAIN",
         800, 586, 23, IVORY);
    drawButton(game, r, "PLAY AGAIN", 510);
    drawButton(game, r, "MAIN MENU", 410);
    drawButton(game, r, "EXIT", 310);
}
