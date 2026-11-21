#include "Renderer2D.h"

#include <cmath>

namespace {
glm::vec4 damageTint(glm::vec4 color, float red)
{
    const glm::vec3 damageColor(0.85f, 0.05f, 0.05f);
    color.r = glm::mix(color.r, damageColor.r, glm::clamp(red, 0.0f, 1.0f));
    color.g = glm::mix(color.g, damageColor.g, glm::clamp(red, 0.0f, 1.0f));
    color.b = glm::mix(color.b, damageColor.b, glm::clamp(red, 0.0f, 1.0f));
    return color;
}
}

void Renderer2D::drawConvex(const std::vector<glm::vec2>& points,
                            const glm::vec4& color,
                            const glm::mat4& model,
                            float red)
{
    if (points.size() < 3) return;

    // The player art stores affine transforms as matrices. Decompose their 2D
    // translation/rotation/scale and draw through the engine's transform stack.
    const glm::vec2 axisX(model[0].x, model[0].y);
    const glm::vec2 axisY(model[1].x, model[1].y);
    const float scaleX = glm::length(axisX);
    if (scaleX < 1.0e-6f) return;
    const float determinant = axisX.x * axisY.y - axisY.x * axisX.y;
    const float scaleY = determinant / scaleX;
    const float angle = glm::degrees(std::atan2(axisX.y, axisX.x));
    const glm::vec2 position(model[3].x, model[3].y);

    renderer_.push();
    renderer_.translate(position);
    renderer_.rotate(angle);
    renderer_.scale({scaleX, scaleY});
    renderer_.polygon(points, damageTint(color, red));
    renderer_.pop();
}

void Renderer2D::drawRect(glm::vec2 center, glm::vec2 size,
                          const glm::vec4& color, const glm::mat4& model,
                          float red)
{
    const glm::vec2 half = size * 0.5f;
    drawConvex({center + glm::vec2(-half.x, -half.y),
                center + glm::vec2( half.x, -half.y),
                center + glm::vec2( half.x,  half.y),
                center + glm::vec2(-half.x,  half.y)}, color, model, red);
}

void Renderer2D::drawEllipse(glm::vec2 center, glm::vec2 radii,
                             const glm::vec4& color, const glm::mat4& model,
                             float red, int segments)
{
    segments = std::max(8, segments);
    std::vector<glm::vec2> points;
    points.reserve(static_cast<size_t>(segments) + 2);
    points.push_back(center);
    for (int i = 0; i <= segments; ++i) {
        const float angle = 2.0f * 3.14159265f * static_cast<float>(i) /
                            static_cast<float>(segments);
        points.push_back(center + glm::vec2(std::cos(angle) * radii.x,
                                            std::sin(angle) * radii.y));
    }
    drawConvex(points, color, model, red);
}

void Renderer2D::drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c,
                              const glm::vec4& color, const glm::mat4& model,
                              float red)
{
    drawConvex({a, b, c}, color, model, red);
}

void Renderer2D::drawLimb(glm::vec2 from, glm::vec2 to, float widthFrom,
                          float widthTo, const glm::vec4& color,
                          const glm::mat4& model, float red)
{
    const glm::vec2 direction = to - from;
    const float length = glm::length(direction);
    if (length < 1.0e-6f) return;
    const glm::vec2 normal(-direction.y / length, direction.x / length);
    drawConvex({from + normal * widthFrom * 0.5f,
                from - normal * widthFrom * 0.5f,
                to   - normal * widthTo   * 0.5f,
                to   + normal * widthTo   * 0.5f}, color, model, red);
    drawEllipse(from, glm::vec2(widthFrom * 0.5f), color, model, red, 20);
    drawEllipse(to, glm::vec2(widthTo * 0.5f), color, model, red, 20);
}
