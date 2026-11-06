#pragma once

#include "core/Game.h"

#include <memory>

// All presentation screens: main menu (+ sound settings), intro cutscene,
// instructions, pause menu and win/lose menu.
// Flow: MainMenu -> Start -> Cutscene (17 s, Enter skips) -> Instructions (3.5 s) -> Playing.
class TempleMenus {
public:
    TempleMenus();
    ~TempleMenus();

    void setup(Game& game);

private:
    struct Cutscene;   // the animated cast of the intro (defined in TempleMenus.cpp)

    void update(Game& game, float dt);
    void updateCutscene(float dt);
    void draw(const Game& game, Renderer& renderer) const;
    void drawCutscene(Renderer& r) const;
    void drawMainMenu(const Game& game, Renderer& r) const;
    void drawSoundSettings(const Game& game, Renderer& r) const;
    void drawInstructions(Renderer& r) const;
    void drawPause(const Game& game, Renderer& r) const;
    void drawGameOver(const Game& game, Renderer& r) const;

    GameState observedState_ = GameState::Playing;
    bool  observed_   = false;
    float stateTime_  = 0.0f;
    bool  soundMenu_  = false;   // Sound Settings page open (inside the main menu)
    std::unique_ptr<Cutscene> cutscene_;
};
