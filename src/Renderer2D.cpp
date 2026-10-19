#include "Renderer2D.h"
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <iostream>

namespace {
const size_t MAX_VERTS = 256;

const char* VERT_SRC = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
uniform mat4 uProjection;
uniform mat4 uModel;
void main() {
    gl_Position = uProjection * uModel * vec4(aPos, 0.0, 1.0);
}
)";

const char* FRAG_SRC = R"(
#version 330 core
out vec4 FragColor;
uniform vec4  uColor;
uniform float uRed;   // 0 = normal colour, 1 = fully red (damage feedback)
void main() {
    vec3 damaged = mix(uColor.rgb, vec3(0.85, 0.05, 0.05), uRed);
    FragColor = vec4(damaged, uColor.a);
}
)";

GLuint compileShader(GLenum type, const char* code)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &code, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, nullptr, log);
        std::cerr << "Shader compile error:\n" << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}
} // namespace

bool Renderer2D::init()
{
    GLuint vs = compileShader(GL_VERTEX_SHADER, VERT_SRC);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
    if (!vs || !fs) return false;

    program_ = glCreateProgram();
    glAttachShader(program_, vs);
    glAttachShader(program_, fs);
    glLinkProgram(program_);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program_, 1024, nullptr, log);
        std::cerr << "Shader link error:\n" << log << "\n";
        return false;
    }

    locProjection_ = glGetUniformLocation(program_, "uProjection");
    locModel_      = glGetUniformLocation(program_, "uModel");
    locColor_      = glGetUniformLocation(program_, "uColor");
    locRed_        = glGetUniformLocation(program_, "uRed");

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, MAX_VERTS * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return true;
}

void Renderer2D::shutdown()
{
    glDeleteBuffers(1, &vbo_);
    glDeleteVertexArrays(1, &vao_);
    glDeleteProgram(program_);
}

void Renderer2D::setProjection(const glm::mat4& projection)
{
    glUseProgram(program_);
    glUniformMatrix4fv(locProjection_, 1, GL_FALSE, glm::value_ptr(projection));
}

void Renderer2D::drawConvex(const std::vector<glm::vec2>& pts, const glm::vec4& color,
                            const glm::mat4& model, float red)
{
    if (pts.size() < 3 || pts.size() > MAX_VERTS) return;
    glUseProgram(program_);
    glUniformMatrix4fv(locModel_, 1, GL_FALSE, glm::value_ptr(model));
    glUniform4fv(locColor_, 1, glm::value_ptr(color));
    glUniform1f(locRed_, red);
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, pts.size() * sizeof(glm::vec2), pts.data());
    glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)pts.size());
}

void Renderer2D::drawRect(glm::vec2 c, glm::vec2 s, const glm::vec4& color,
                          const glm::mat4& model, float red)
{
    glm::vec2 h = s * 0.5f;
    drawConvex({ c + glm::vec2(-h.x, -h.y), c + glm::vec2(h.x, -h.y),
                 c + glm::vec2(h.x, h.y),   c + glm::vec2(-h.x, h.y) },
               color, model, red);
}

void Renderer2D::drawEllipse(glm::vec2 c, glm::vec2 r, const glm::vec4& color,
                             const glm::mat4& model, float red, int segments)
{
    std::vector<glm::vec2> pts;
    pts.reserve(segments + 2);
    pts.push_back(c); // fan centre
    for (int i = 0; i <= segments; ++i) {
        float a = 2.0f * 3.14159265f * (float)i / (float)segments;
        pts.push_back(c + glm::vec2(std::cos(a) * r.x, std::sin(a) * r.y));
    }
    drawConvex(pts, color, model, red);
}

void Renderer2D::drawTriangle(glm::vec2 a, glm::vec2 b, glm::vec2 c, const glm::vec4& color,
                              const glm::mat4& model, float red)
{
    drawConvex({ a, b, c }, color, model, red);
}

void Renderer2D::drawLimb(glm::vec2 from, glm::vec2 to, float wf, float wt,
                          const glm::vec4& color, const glm::mat4& model, float red)
{
    glm::vec2 d = to - from;
    float len = glm::length(d);
    if (len < 1e-6f) return;
    glm::vec2 n = glm::vec2(-d.y, d.x) / len;
    drawConvex({ from + n * wf * 0.5f, from - n * wf * 0.5f,
                 to - n * wt * 0.5f,   to + n * wt * 0.5f },
               color, model, red);
    drawEllipse(from, glm::vec2(wf * 0.5f), color, model, red, 20);
    drawEllipse(to,   glm::vec2(wt * 0.5f), color, model, red, 20);
}
