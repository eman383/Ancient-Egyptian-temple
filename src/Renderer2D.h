#pragma once

#include "render/Renderer.h"
#include <glm/glm.hpp>
#include <vector>

// Small compatibility adapter for the existing player model geometry.
// It forwards all drawing to the shared engine Renderer; it owns no GL state.
class Renderer2D {
public:
    explicit Renderer2D(Renderer& renderer) : renderer_(renderer) {}

    void drawConvex(const std::vector<glm::vec2>& pts, const glm::vec4& color,
                    const glm::mat4& model, float red = 0.0f);
    void drawRect(glm::vec2 center, glm::vec2 size, const glm::vec4& color,
                  const glm::mat4& model, float red = 0.0f);
    void drawEllipse(glm::vec2 center, glm::vec2 radii, const glm::vec4& color,
                     const glm::mat4& model, float red = 0.0f, int segments = 32);
    void drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, const glm::vec4& color,
                      const glm::mat4& model, float red = 0.0f);
    void drawLimb(glm::vec2 from, glm::vec2 to, float widthFrom, float widthTo,
                  const glm::vec4& color, const glm::mat4& model, float red = 0.0f);

private:
    Renderer& renderer_;
};
