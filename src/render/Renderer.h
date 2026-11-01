#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "Shader.h"

// 2D shape renderer with a transform stack.
//
// How to build a model (example: a character):
//   r.push();
//     r.translate({x, y});          // feet-centre of the character
//     r.rect({0, 60}, {40, 80}, bodyColor);   // body
//     r.circle({0, 120}, 20, skinColor);      // head
//     r.push();
//       r.translate({20, 70});      // shoulder
//       r.rotate(armAngle);         // animate the arm
//       r.rect({0, -20}, {10, 40}, armColor);
//     r.pop();
//   r.pop();
//
// Every shape is drawn in the CURRENT local space, so children move with parents.
class Renderer {
public:
    bool init();
    void shutdown();

    // Call once per frame before drawing. Sets the camera (projection).
    void begin(const glm::mat4& projection);

    // ---- transform stack ----
    void push();
    void pop();
    void translate(glm::vec2 t);
    void rotate(float degrees);          // counter-clockwise
    void scale(glm::vec2 s);             // negative x flips horizontally (used for enemies)

    // ---- shapes (centre-based, drawn in current local space) ----
    void rect(glm::vec2 center, glm::vec2 size, glm::vec4 color);
    void triangle(glm::vec2 center, glm::vec2 size, glm::vec4 color);   // apex up
    void circle(glm::vec2 center, float radius, glm::vec4 color);
    void ellipse(glm::vec2 center, glm::vec2 radii, glm::vec4 color);
    // Custom filled convex polygon from local-space points (triangle fan)
    void polygon(const std::vector<glm::vec2>& pts, glm::vec4 color);
    // Rect given by its bottom-left corner (handy for lanes / UI)
    void rectBL(glm::vec2 bottomLeft, glm::vec2 size, glm::vec4 color);

private:
    struct Mesh { GLuint vao = 0, vbo = 0; GLsizei count = 0; GLenum mode = GL_TRIANGLES; };
    Mesh makeMesh(const std::vector<float>& xy, GLenum mode);
    void draw(const Mesh& m, glm::vec2 pos, glm::vec2 size, glm::vec4 color);

    Shader shader;
    Mesh quadMesh, triMesh, circleMesh, polyMesh;
    std::vector<glm::mat4> stack;
    glm::mat4 current{1.0f};
};