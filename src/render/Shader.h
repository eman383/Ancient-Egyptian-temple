#pragma once
#include <glad/gl.h>
#include <glm/glm.hpp>

// Small wrapper around a GL shader program.
class Shader {
public:
    bool build(const char* vertexSrc, const char* fragmentSrc);
    void use() const { glUseProgram(program); }
    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec4(const char* name, const glm::vec4& v) const;
    void destroy();
private:
    GLuint program = 0;
    static GLuint compile(GLenum type, const char* src);
};