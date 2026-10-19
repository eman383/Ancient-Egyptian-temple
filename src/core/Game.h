#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <functional>
#include <vector>
#include "Input.h"
#include "LaneGrid.h"
#include "render/Renderer.h"

enum class GameState { MainMenu, Cutscene, Playing, Paused, GameOver };

// Draw layers for onDraw(). Drawn in this order, on top of the environment + lanes.
//   Entities : players, enemies, collectibles
//   Effects  : projectiles, hit effects, health bars
//   Hud      : cards, coin counter (scene states only: Playing / Paused / GameOver)
//   Menus    : main menu, cutscene, pause menu, win/lose menu (drawn in EVERY state)
enum class Layer { Entities = 0, Effects = 1, Hud = 2, Menus = 3 };

class Game {
public:
    using UpdateFn = std::function<void(float dt)>;
    using DrawFn   = std::function<void(Renderer&)>;
    using ResetFn  = std::function<void()>;

    bool init();
    void run();
    void shutdown();

    // ---------------- commands: call these from menus / gameplay code ----------------
    void startGame();        // MainMenu  -> Cutscene   (Start button)
    void finishCutscene();   // Cutscene  -> Playing    (cutscene ended or skipped)
    void pause();            // Playing   -> Paused     (Esc does this automatically)
    void resume();           // Paused    -> Playing    (Continue button)
    void restart();          // fresh game, straight to Playing (Restart buttons)
    void goToMainMenu();     // fresh game, back to the main menu
    void win();              // Playing   -> GameOver (player won)
    void lose();             // Playing   -> GameOver (enemy reached home base): everything freezes
    void requestExit();      // closes the application (Exit button)

    // ---------------- queries ----------------
    GameState state() const { return currentState; }
    bool  isPlaying() const { return currentState == GameState::Playing; }
    bool  playerWon() const { return won; }          // valid while state == GameOver
    float gameTime() const  { return gameplayTime; } // seconds of actual play (freezes on pause)

    // ---------------- hooks: register once at startup, no need to edit Game.cpp ------
    void onGameplayUpdate(UpdateFn fn); // called only while Playing (B, C: movement, combat, waves)
    void onAlwaysUpdate(UpdateFn fn);   // called every frame in every state (D: menus, cutscene)
    void onDraw(Layer layer, DrawFn fn);
    void onReset(ResetFn fn);           // called on every restart / return to menu (clear your lists!)

    void reset();                        // runs all reset hooks (called by the commands above)
    void setState(GameState s);

    Renderer renderer;
    Input input;
    LaneGrid lanes;

private:
    void update(float dt);
    void draw();
    void drawScene();
    void runDraw(Layer layer);
    void updateViewport();        // letterboxing + mouse conversion setup
    void updateMouse();

    static void keyCallback(GLFWwindow*, int key, int, int action, int);
    static void mouseButtonCallback(GLFWwindow*, int button, int action, int);

    GLFWwindow* window = nullptr;
    GameState currentState = GameState::Playing;
    bool  won = false;
    float sceneTime = 0.0f;       // drives scenery animation; stops on Paused / GameOver
    float gameplayTime = 0.0f;    // advances only while Playing
    // letterboxed viewport in framebuffer pixels
    int vpX = 0, vpY = 0, vpW = 1, vpH = 1;
    int fbW = 1, fbH = 1;

    std::vector<UpdateFn> gameplayUpdates, alwaysUpdates;
    std::vector<DrawFn>   drawHooks[4];
    std::vector<ResetFn>  resetHooks;
};