#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

static const char* VERT = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 uProj;
uniform mat4 uModel;
void main() {
    gl_Position = uProj * uModel * vec4(aPos, 0.0, 1.0);
}
)";

static const char* FRAG = R"(
#version 330 core
out vec4 FragColor;
uniform vec4 uColor;
void main() {
    FragColor = uColor;
}
)";

Renderer::Mesh Renderer::makeMesh(const std::vector<float>& xy, GLenum mode) {
    Mesh m;
    m.mode = mode;
    m.count = static_cast<GLsizei>(xy.size() / 2);
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, xy.size() * sizeof(float), xy.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return m;
}

bool Renderer::init() {
    if (!shader.build(VERT, FRAG)) return false;

    // unit square centred on the origin (two triangles as a fan)
    quadMesh = makeMesh({-0.5f,-0.5f,  0.5f,-0.5f,  0.5f,0.5f,  -0.5f,0.5f}, GL_TRIANGLE_FAN);
    // unit triangle, apex up
    triMesh = makeMesh({-0.5f,-0.5f,  0.5f,-0.5f,  0.0f,0.5f}, GL_TRIANGLES);
    // unit circle (radius 0.5) as a fan
    std::vector<float> c = {0.0f, 0.0f};
    const int SEG = 40;
    for (int i = 0; i <= SEG; i++) {
        float a = 6.2831853f * i / SEG;
        c.push_back(0.5f * std::cos(a));
        c.push_back(0.5f * std::sin(a));
    }
    circleMesh = makeMesh(c, GL_TRIANGLE_FAN);
    // dynamic mesh reused for custom polygons
    polyMesh = makeMesh({0,0, 0,0, 0,0}, GL_TRIANGLE_FAN);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;
}

void Renderer::shutdown() {
    for (Mesh* m : {&quadMesh, &triMesh, &circleMesh, &polyMesh}) {
        glDeleteVertexArrays(1, &m->vao);
        glDeleteBuffers(1, &m->vbo);
    }
    shader.destroy();
}

void Renderer::begin(const glm::mat4& projection) {
    shader.use();
    shader.setMat4("uProj", projection);
    stack.clear();
    current = glm::mat4(1.0f);
}

void Renderer::push() { stack.push_back(current); }
void Renderer::pop()  { if (!stack.empty()) { current = stack.back(); stack.pop_back(); } }
void Renderer::translate(glm::vec2 t) { current = glm::translate(current, glm::vec3(t, 0.0f)); }
void Renderer::rotate(float deg)      { current = glm::rotate(current, glm::radians(deg), glm::vec3(0, 0, 1)); }
void Renderer::scale(glm::vec2 s)     { current = glm::scale(current, glm::vec3(s, 1.0f)); }

void Renderer::draw(const Mesh& m, glm::vec2 pos, glm::vec2 size, glm::vec4 color) {
    glm::mat4 model = glm::translate(current, glm::vec3(pos, 0.0f));
    model = glm::scale(model, glm::vec3(size, 1.0f));
    shader.setMat4("uModel", model);
    shader.setVec4("uColor", color);
    glBindVertexArray(m.vao);
    glDrawArrays(m.mode, 0, m.count);
}

void Renderer::rect(glm::vec2 c, glm::vec2 size, glm::vec4 col)        { draw(quadMesh, c, size, col); }
void Renderer::triangle(glm::vec2 c, glm::vec2 size, glm::vec4 col)    { draw(triMesh, c, size, col); }
void Renderer::circle(glm::vec2 c, float r, glm::vec4 col)             { draw(circleMesh, c, glm::vec2(2 * r), col); }
void Renderer::ellipse(glm::vec2 c, glm::vec2 rr, glm::vec4 col)       { draw(circleMesh, c, rr * 2.0f, col); }
void Renderer::rectBL(glm::vec2 bl, glm::vec2 size, glm::vec4 col)     { draw(quadMesh, bl + size * 0.5f, size, col); }

void Renderer::polygon(const std::vector<glm::vec2>& pts, glm::vec4 color) {
    if (pts.size() < 3) return;
    std::vector<float> xy;
    xy.reserve(pts.size() * 2);
    for (auto& p : pts) { xy.push_back(p.x); xy.push_back(p.y); }
    glBindBuffer(GL_ARRAY_BUFFER, polyMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, xy.size() * sizeof(float), xy.data(), GL_DYNAMIC_DRAW);
    polyMesh.count = static_cast<GLsizei>(pts.size());
    draw(polyMesh, glm::vec2(0), glm::vec2(1), color);
}