// Standalone character test window (does NOT replace your main.cpp).
//   SPACE = defense player takes a hit     R = reset     ESC = quit
// Add new characters here as you build them to see them side by side.
#define GLAD_GL_IMPLEMENTATION      // exactly one .cpp per executable defines this
#include <glad/gl.h>                // must come before glfw3.h
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "DefensePlayer.h"
#include "Renderer2D.h"

int main()
{
    const int W = 1280, H = 720;

    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(W, H, "Character Test", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (gladLoadGL(glfwGetProcAddress) == 0) {
        std::cerr << "Failed to initialize OpenGL context\n";
        return -1;
    }
    glViewport(0, 0, W, H);

    Renderer2D renderer;
    if (!renderer.init()) return -1;
    renderer.setProjection(glm::ortho(0.0f, (float)W, 0.0f, (float)H));

    DefensePlayer defender({420.0f, 150.0f}, 330.0f);
    std::cout << "SPACE = hit the defense player, R = reset, ESC = quit\n";

    bool spaceWas = false, rWas = false;
    double last = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        bool space = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool rKey  = glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS;
        if (space && !spaceWas) defender.takeHit();
        if (rKey && !rWas) defender.reset();
        spaceWas = space;
        rWas = rKey;

        defender.update(dt);

        glClearColor(0.42f, 0.74f, 0.20f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 id(1.0f);
        renderer.drawRect({W * 0.5f, 270.0f}, {W, 400.0f}, glm::vec4(0.30f, 0.55f, 0.15f, 1.0f), id);
        renderer.drawRect({W * 0.5f, 70.0f}, {W, 140.0f}, glm::vec4(0.25f, 0.18f, 0.12f, 1.0f), id);

        defender.draw(renderer);

        glfwSwapBuffers(window);
    }

    renderer.shutdown();
    glfwTerminate();
    return 0;
}
