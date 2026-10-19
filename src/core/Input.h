#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// Frame-based input state. "pressed" is true only on the frame the button went down.
struct Input {
    glm::vec2 mouse{0.0f};        // in VIRTUAL coordinates (see Config.h), origin bottom-left
    bool mouseDown[3]    = {false, false, false};
    bool mousePressed[3] = {false, false, false};
    bool keyDown[512]    = {false};
    bool keyPressed[512] = {false};

    bool leftClicked() const { return mousePressed[0]; }
    bool key(int glfwKey) const            { return keyDown[glfwKey]; }
    bool keyJustPressed(int glfwKey) const { return keyPressed[glfwKey]; }

    // Gameplay commands (B uses these). True only on the frame the key goes down.
    bool jumpPressed() const { return keyPressed[GLFW_KEY_W] || keyPressed[GLFW_KEY_UP]; }
    bool duckPressed() const { return keyPressed[GLFW_KEY_S] || keyPressed[GLFW_KEY_DOWN]; }

    // called by Game at the end of every frame
    void endFrame() {
        for (bool& b : mousePressed) b = false;
        for (bool& b : keyPressed) b = false;
    }
};