// Standalone test window for the Long Range (archer) player. Does NOT replace main.cpp.
//   W / UP = jump     S / DOWN = duck     SPACE = she takes a hit
//   E = toggle "touching an enemy" (stops walking, keeps shooting)
//   R = reset         ESC = quit
// The brown box on the right is a dummy target: arrows that reach it play their impact animation.
#define GLAD_GL_IMPLEMENTATION      // exactly one .cpp per executable defines this
#include <glad/gl.h>                // must come before glfw3.h
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "LongRangePlayer.h"
#include "Renderer2D.h"

int main()
{
    const int W = 1280, H = 720;

    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(W, H, "Long Range Test", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cerr << "Failed to initialize OpenGL context\n"; return -1; }
    glViewport(0, 0, W, H);

    Renderer2D renderer;
    if (!renderer.init()) return -1;
    renderer.setProjection(glm::ortho(0.0f, (float)W, 0.0f, (float)H));

    const float targetX = 1120.0f;
    auto makeArcher = [&]() {
        LongRangePlayer a({80.0f, 150.0f}, 330.0f);
        a.setArrowMaxX((float)W + 100.0f);
        return a;
    };
    LongRangePlayer archer = makeArcher();
    std::cout << "W/UP = jump, S/DOWN = duck, SPACE = hit her, E = engage, R = reset, ESC = quit\n";

    bool prev[6] = {false, false, false, false, false, false};   // edge detection
    bool engaged = false;
    double last = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        bool k[6] = {
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS,
            false };
        if (k[0] && !prev[0]) archer.jump();
        if (k[1] && !prev[1]) archer.duck();
        if (k[2] && !prev[2]) archer.takeHit();
        if (k[3] && !prev[3]) engaged = !engaged;
        if (k[4] && !prev[4]) { archer = makeArcher(); engaged = false; }
        for (int i = 0; i < 6; ++i) prev[i] = k[i];

        archer.setEngaged(engaged);
        archer.update(dt);

        // arrow vs. dummy target (your game does this against real enemies)
        auto& arrows = archer.arrows();
        for (size_t i = 0; i < arrows.size(); ++i)
            if (!arrows[i].hit && archer.arrowTip(arrows[i]).x >= targetX - 40.0f)
                archer.arrowHit(i);

        // she walks off the right edge -> bring her back so the test can go on
        if (archer.position().x > W - 150.0f && !engaged) archer = makeArcher();

        glClearColor(0.96f, 0.84f, 0.58f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 id(1.0f);
        renderer.drawRect({W * 0.5f, 75.0f}, {(float)W, 150.0f}, glm::vec4(0.72f, 0.55f, 0.30f, 1.0f), id);
        renderer.drawRect({targetX, 270.0f}, {70.0f, 250.0f}, glm::vec4(0.45f, 0.30f, 0.18f, 1.0f), id);   // dummy
        renderer.drawEllipse({targetX, 330.0f}, {50.0f, 50.0f}, glm::vec4(0.85f, 0.15f, 0.15f, 1.0f), id);

        archer.draw(renderer);

        glfwSwapBuffers(window);
    }

    renderer.shutdown();
    glfwTerminate();
    return 0;
}
