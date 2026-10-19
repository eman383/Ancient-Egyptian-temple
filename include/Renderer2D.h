#pragma once
// GLAD 2 (must come BEFORE glfw3.h). The guard stops a second inclusion, which
// would break a .cpp that defines GLAD_GL_IMPLEMENTATION (e.g. your main.cpp).
#ifndef GLAD_GL_H_
#include <glad/gl.h>
#endif
#include <glm/glm.hpp>
#include <vector>

// Minimal 2D shape renderer (modern OpenGL 3.3 core).
// Every shape is drawn as a triangle fan, so each polygon passed to
// drawConvex() must be CONVEX (non-convex shapes: split into convex pieces).
class Renderer2D {
public:
    bool init();   // compiles the built-in shaders (needs a current GL context)
    void shutdown();
    void setProjection(const glm::mat4& projection);

    // 'model' places the shape in the world (translate/rotate/scale).
    // 'red' (0..1) blends the colour toward red for damage feedback.
    void drawConvex(const std::vector<glm::vec2>& pts, const glm::vec4& color,
                    const glm::mat4& model, float red = 0.0f);
    void drawRect(glm::vec2 center, glm::vec2 size, const glm::vec4& color,
                  const glm::mat4& model, float red = 0.0f);
    void drawEllipse(glm::vec2 center, glm::vec2 radii, const glm::vec4& color,
                     const glm::mat4& model, float red = 0.0f, int segments = 32);
    void drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, const glm::vec4& color,
                      const glm::mat4& model, float red = 0.0f);
    // Tapered "capsule" between two points (arms, legs).
    void drawLimb(glm::vec2 from, glm::vec2 to, float widthFrom, float widthTo,
                  const glm::vec4& color, const glm::mat4& model, float red = 0.0f);

private:
    GLuint program_ = 0, vao_ = 0, vbo_ = 0;
    GLint locProjection_ = -1, locModel_ = -1, locColor_ = -1, locRed_ = -1;
};
