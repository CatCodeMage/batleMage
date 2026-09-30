#include "gfx/Shader.h"

#include <stdexcept>
#include <vector>

#include <glm/gtc/type_ptr.hpp>

namespace gfx {

namespace {

GLuint compile(GLenum type, const char* src, const char* label) {
    const GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? static_cast<std::size_t>(len) : 1u, 0);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        glDeleteShader(s);
        throw std::runtime_error(std::string("Error compilando el shader ") + label + ": " + log.data());
    }
    return s;
}

}  // namespace

Shader::Shader(const char* vertexSrc, const char* fragmentSrc) {
    const GLuint v = compile(GL_VERTEX_SHADER, vertexSrc, "vertex");
    const GLuint f = compile(GL_FRAGMENT_SHADER, fragmentSrc, "fragment");

    program_ = glCreateProgram();
    glAttachShader(program_, v);
    glAttachShader(program_, f);
    glLinkProgram(program_);
    glDeleteShader(v);
    glDeleteShader(f);

    GLint ok = 0;
    glGetProgramiv(program_, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(len > 1 ? static_cast<std::size_t>(len) : 1u, 0);
        glGetProgramInfoLog(program_, len, nullptr, log.data());
        glDeleteProgram(program_);
        program_ = 0;
        throw std::runtime_error(std::string("Error enlazando el shader: ") + log.data());
    }
}

Shader::~Shader() {
    if (program_) glDeleteProgram(program_);
}

void Shader::use() const {
    glUseProgram(program_);
}

GLint Shader::location(const char* name) const {
    auto it = cache_.find(name);
    if (it != cache_.end()) return it->second;
    const GLint loc = glGetUniformLocation(program_, name);
    cache_.emplace(name, loc);
    return loc;
}

void Shader::setInt(const char* name, int v) const { glUniform1i(location(name), v); }
void Shader::setFloat(const char* name, float v) const { glUniform1f(location(name), v); }
void Shader::setVec3(const char* name, const glm::vec3& v) const { glUniform3fv(location(name), 1, glm::value_ptr(v)); }
void Shader::setMat3(const char* name, const glm::mat3& m) const { glUniformMatrix3fv(location(name), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setMat4(const char* name, const glm::mat4& m) const { glUniformMatrix4fv(location(name), 1, GL_FALSE, glm::value_ptr(m)); }
void Shader::setVec3Array(const char* name, const glm::vec3* v, int count) const {
    if (count > 0) glUniform3fv(location(name), count, glm::value_ptr(v[0]));
}

}  // namespace gfx
