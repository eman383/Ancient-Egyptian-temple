// Standalone test window for the Melee (pistol) player. Does NOT replace main.cpp.
//   W / UP = jump     S / DOWN = duck     SPACE = he takes a hit
//   E = toggle "touching an enemy" (stops walking, keeps shooting)
//   R = reset         ESC = quit
// The brown box on the right is a dummy target. Every volley sends a bullet out of the gun: if it reaches
// the dummy the dummy flashes red; if the dummy is too far, the bullet drops to the ground. Press E to
// stop him anywhere (to watch bullets drop).
#define GLAD_GL_IMPLEMENTATION      // exactly one .cpp per executable defines this
#include <glad/gl.h>                // must come before glfw3.h
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include "MeleePlayer.h"
#include "Renderer2D.h"

int main()
{
    const int W = 1280, H = 720;

    if (!glfwInit()) { std::cerr << "Failed to init GLFW\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(W, H, "Melee Test", nullptr, nullptr);
    if (!window) { std::cerr << "Failed to create window\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (gladLoadGL(glfwGetProcAddress) == 0) { std::cerr << "Failed to initialize OpenGL context\n"; return -1; }
    glViewport(0, 0, W, H);

    Renderer2D renderer;
    if (!renderer.init()) return -1;
    renderer.setProjection(glm::ortho(0.0f, (float)W, 0.0f, (float)H));

    const float targetX = 900.0f;      // dummy enemy
    float targetFlash = 0.0f;
    int   targetHits  = 0;

    auto makePlayer = [&]() { return MeleePlayer({80.0f, 150.0f}, 330.0f); };
    MeleePlayer player = makePlayer();
    std::cout << "W/UP = jump, S/DOWN = duck, SPACE = hit him, E = engage, R = reset, ESC = quit\n";

    bool prev[5] = {false, false, false, false, false};
    bool manualEngage = false;
    double last = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - last);
        last = now;

        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        bool k[5] = {
            glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS,
            glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS };
        if (k[0] && !prev[0]) player.jump();
        if (k[1] && !prev[1]) player.duck();
        if (k[2] && !prev[2]) player.takeHit();
        if (k[3] && !prev[3]) manualEngage = !manualEngage;
        if (k[4] && !prev[4]) { player = makePlayer(); manualEngage = false; targetHits = 0; }
        for (int i = 0; i < 5; ++i) prev[i] = k[i];

        // "collision" with the dummy: he is touching it when the target is inside his reach
        float dx = targetX - 35.0f - player.position().x;            // gap to the dummy's near face
        bool inReach = dx <= player.reach();
        player.setEngaged(manualEngage || inReach);
        player.update(dt);

        player.consumeAttack();                                      // "a bullet was fired" (sound effects go here)
        if (player.bulletHits(targetX - 35.0f, targetX + 35.0f)) {   // what your game does for real enemies
            ++targetHits;
            targetFlash = 0.12f;
        }
        if (targetFlash > 0.0f) targetFlash -= dt;

        // he walked off the right edge (nothing in his way) -> bring him back
        if (player.position().x > W - 100.0f) player = makePlayer();

        glClearColor(0.96f, 0.84f, 0.58f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 id(1.0f);
        renderer.drawRect({W * 0.5f, 75.0f}, {(float)W, 150.0f}, glm::vec4(0.72f, 0.55f, 0.30f, 1.0f), id);
        float red = targetFlash > 0.0f ? 1.0f : 0.0f;
        renderer.drawRect({targetX, 270.0f}, {70.0f, 250.0f}, glm::vec4(0.45f, 0.30f, 0.18f, 1.0f), id, red);
        renderer.drawEllipse({targetX, 330.0f}, {50.0f, 50.0f}, glm::vec4(0.85f, 0.85f, 0.80f, 1.0f), id, red);
        for (int i = 0; i < targetHits && i < 12; ++i)                 // one dot per bullet that landed
            renderer.drawEllipse({targetX - 28.0f + (i % 4) * 18.0f, 250.0f - (i / 4) * 18.0f}, {5.0f, 5.0f},
                                 glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), id);

        player.draw(renderer);

        glfwSwapBuffers(window);
    }

    renderer.shutdown();
    glfwTerminate();
    return 0;
}
