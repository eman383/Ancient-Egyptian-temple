#pragma once

#include "core/Game.h"

#include <string>

// All presentation screens live here. Nothing in the player or enemy logic is modified.
// Timeline: Cutscene (15 s, Enter skips) -> MainMenu -> Instructions (3.5 s)
//           -> Playing -> Paused/GameOver as normal.
class TempleMenus {
public:
    void setup(Game& game);
    ~TempleMenus();

private:
    void update(Game& game, float dt);
    void draw(const Game& game, Renderer& renderer) const;
    void drawCutscene(Renderer& r) const;
    void drawMainMenu(const Game& game, Renderer& r) const;
    void drawInstructions(Renderer& r) const;
    void drawPause(const Game& game, Renderer& r) const;
    void drawGameOver(const Game& game, Renderer& r) const;

    void syncMusic(GameState state);
    void stopMusic();

    GameState observedState_ = GameState::Playing;
    bool observed_ = false;
    float stateTime_ = 0.0f;
    bool musicEnabled_ = true;
    bool musicPlaying_ = false;
    std::string audioFile_;
};
