#include "Game.h"
#include "Config.h"
#include "game/HomeBase.h"
#include "game/Environment.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <cmath>

// ---- Dev switches (set both for the final submission) ----
// START_STATE : D changes this to MainMenu once the main menu exists.
// DEV_KEYS    : 1..6 jump between states, R restarts. Set to false before submitting.
static const GameState START_STATE = GameState::Playing;
static const bool      DEV_KEYS    = true;

static const char* stateName(GameState s) {
    switch (s) {
        case GameState::MainMenu: return "MainMenu";
        case GameState::Cutscene: return "Cutscene";
        case GameState::Playing:  return "Playing";
        case GameState::Paused:   return "Paused";
        case GameState::GameOver: return "GameOver";
    }
    return "?";
}

bool Game::init() {
    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return false; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1280, 720, "Tower Defense", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return false; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cerr << "Failed to init GLAD\n"; return false; }

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);

    if (!renderer.init()) return false;
    currentState = START_STATE;
    return true;
}

void Game::keyCallback(GLFWwindow* w, int key, int, int action, int) {
    Game* g = static_cast<Game*>(glfwGetWindowUserPointer(w));
    if (key < 0 || key >= 512) return;
    if (action == GLFW_PRESS)   { g->input.keyDown[key] = true;  g->input.keyPressed[key] = true; }
    if (action == GLFW_RELEASE) { g->input.keyDown[key] = false; }
}

void Game::mouseButtonCallback(GLFWwindow* w, int button, int action, int) {
    Game* g = static_cast<Game*>(glfwGetWindowUserPointer(w));
    if (button < 0 || button > 2) return;
    if (action == GLFW_PRESS)   { g->input.mouseDown[button] = true;  g->input.mousePressed[button] = true; }
    if (action == GLFW_RELEASE) { g->input.mouseDown[button] = false; }
}

void Game::setState(GameState s) {
    if (s == currentState) return;
    currentState = s;
    if (DEV_KEYS) std::cout << "[state] " << stateName(s) << "\n";
}

// ---- hooks ----
void Game::onGameplayUpdate(UpdateFn fn) { gameplayUpdates.push_back(std::move(fn)); }
void Game::onAlwaysUpdate(UpdateFn fn)   { alwaysUpdates.push_back(std::move(fn)); }
void Game::onDraw(Layer layer, DrawFn fn){ drawHooks[static_cast<int>(layer)].push_back(std::move(fn)); }
void Game::onReset(ResetFn fn)           { resetHooks.push_back(std::move(fn)); }

void Game::runDraw(Layer layer) {
    for (auto& fn : drawHooks[static_cast<int>(layer)]) fn(renderer);
}

// Brings everything back to a fresh game. Does NOT change the state by itself.
void Game::reset() {
    lanes.reset();
    won = false;
    gameplayTime = 0.0f;
    sceneTime = 0.0f;
    for (auto& fn : resetHooks) fn();      // B / C / D clear their own lists here
}

// ---- commands ----
void Game::startGame()      { reset(); setState(GameState::Cutscene); }
void Game::finishCutscene() { if (currentState == GameState::Cutscene) setState(GameState::Playing); }
void Game::pause()          { if (currentState == GameState::Playing)  setState(GameState::Paused); }
void Game::resume()         { if (currentState == GameState::Paused)   setState(GameState::Playing); }
void Game::restart()        { reset(); setState(GameState::Playing); }
void Game::goToMainMenu()   { reset(); setState(GameState::MainMenu); }
void Game::win()            { if (currentState == GameState::Playing) { won = true;  setState(GameState::GameOver); } }
void Game::lose()           { if (currentState == GameState::Playing) { won = false; setState(GameState::GameOver); } }
void Game::requestExit()    { if (window) glfwSetWindowShouldClose(window, GLFW_TRUE); }

// Keep 16:9 and add black bars when the window has a different shape.
void Game::updateViewport() {
    glfwGetFramebufferSize(window, &fbW, &fbH);
    if (fbW < 1 || fbH < 1) return;
    float target = VIRTUAL_W / VIRTUAL_H;
    float actual = static_cast<float>(fbW) / fbH;
    if (actual > target) { vpH = fbH; vpW = static_cast<int>(fbH * target); }
    else                 { vpW = fbW; vpH = static_cast<int>(fbW / target); }
    vpX = (fbW - vpW) / 2;
    vpY = (fbH - vpH) / 2;
}

// Window pixels -> virtual coordinates (handles letterboxing and HiDPI).
void Game::updateMouse() {
    double px, py;
    glfwGetCursorPos(window, &px, &py);
    int winW, winH;
    glfwGetWindowSize(window, &winW, &winH);
    if (winW < 1 || winH < 1) return;
    float fx = static_cast<float>(px) * fbW / winW;
    float fy = fbH - static_cast<float>(py) * fbH / winH;   // flip: GLFW y is from the top
    input.mouse.x = (fx - vpX) / vpW * VIRTUAL_W;
    input.mouse.y = (fy - vpY) / vpH * VIRTUAL_H;
}

void Game::update(float dt) {
    updateMouse();

    // Dev shortcuts to test the state machine without menus
    if (DEV_KEYS) {
        if (input.keyJustPressed(GLFW_KEY_1)) setState(GameState::MainMenu);
        if (input.keyJustPressed(GLFW_KEY_2)) setState(GameState::Cutscene);
        if (input.keyJustPressed(GLFW_KEY_3)) setState(GameState::Playing);
        if (input.keyJustPressed(GLFW_KEY_4)) pause();
        if (input.keyJustPressed(GLFW_KEY_5)) lose();
        if (input.keyJustPressed(GLFW_KEY_6)) win();
        if (input.keyJustPressed(GLFW_KEY_R)) restart();
    }

    // Esc toggles pause
    if (input.keyJustPressed(GLFW_KEY_ESCAPE)) {
        if (currentState == GameState::Playing)     pause();
        else if (currentState == GameState::Paused) resume();
    }

    // scenery animation freezes when paused or when the game is over
    if (currentState != GameState::Paused && currentState != GameState::GameOver)
        sceneTime += dt;

    for (auto& fn : alwaysUpdates) fn(dt);

    if (currentState == GameState::Playing) {
        gameplayTime += dt;
        lanes.update(input.mouse, input.leftClicked());    // before the hooks, so they can read it

        // TEMPORARY test: shows which lanes would jump / duck. B replaces this.
        if (input.jumpPressed() || input.duckPressed()) {
            std::cout << (input.jumpPressed() ? "JUMP" : "DUCK") << " in lanes:";
            for (int l : lanes.selectedLanes()) std::cout << " " << l;
            if (!lanes.anySelected()) std::cout << " (none selected)";
            std::cout << "\n";
        }

        for (auto& fn : gameplayUpdates) fn(dt);
    } else {
        lanes.clearHover();
    }
}

void Game::drawScene() {
    Renderer& r = renderer;
    const bool sceneVisible = currentState == GameState::Playing ||
                              currentState == GameState::Paused  ||
                              currentState == GameState::GameOver;

    if (sceneVisible) {
        // backdrop -> lanes -> decor -> home base -> highlight -> hook layers
        drawBackdrop(r, sceneTime);
        lanes.drawLanes(r);
        drawFieldDecor(r, sceneTime);
        drawHomeBase(r, sceneTime);
        lanes.drawHighlights(r);

        runDraw(Layer::Entities);
        runDraw(Layer::Effects);
        runDraw(Layer::Hud);

        // dim / tint so the frozen scene reads as paused or finished
        if (currentState == GameState::Paused)
            r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.0f, 0.0f, 0.0f, 0.55f});
        else if (currentState == GameState::GameOver)
            r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, won ? glm::vec4(0.1f, 0.6f, 0.2f, 0.40f)
                                                         : glm::vec4(0.8f, 0.1f, 0.1f, 0.50f));
    } else if (currentState == GameState::MainMenu) {
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.10f, 0.12f, 0.30f, 1});   // placeholder: D draws the real menu
    } else {
        r.rectBL({0, 0}, {VIRTUAL_W, VIRTUAL_H}, {0.30f, 0.20f, 0.08f, 1});   // placeholder: D draws the real cutscene
    }

    runDraw(Layer::Menus);     // menus, cutscene, pause/win/lose screens (every state)
}

void Game::draw() {
    updateViewport();

    // black bars
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, fbW, fbH);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // game area
    glViewport(vpX, vpY, vpW, vpH);
    glEnable(GL_SCISSOR_TEST);
    glScissor(vpX, vpY, vpW, vpH);
    glClearColor(0.1f, 0.1f, 0.15f, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    glm::mat4 proj = glm::ortho(0.0f, VIRTUAL_W, 0.0f, VIRTUAL_H);
    renderer.begin(proj);
    drawScene();
}

void Game::run() {
    double last = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = static_cast<float>(now - last);
        last = now;
        if (dt > 0.05f) dt = 0.05f;      // avoid huge jumps (window drag, breakpoints)

        glfwPollEvents();
        update(dt);
        draw();
        input.endFrame();
        glfwSwapBuffers(window);
    }
}

void Game::shutdown() {
    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
}