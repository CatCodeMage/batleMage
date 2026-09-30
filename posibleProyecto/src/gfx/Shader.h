#pragma once
#include <string>
#include <unordered_map>

#include <GL/glew.h>
#include <glm/glm.hpp>

namespace gfx {

// Programa de shaders (vertex + fragment) con RAII: se libera solo al destruirse.
class Shader {
public:
    Shader(const char* vertexSrc, const char* fragmentSrc);   // lanza std::runtime_error si falla
    ~Shader();
    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;

    void use() const;

    void setInt(const char* name, int v) const;
    void setFloat(const char* name, float v) const;
    void setVec3(const char* name, const glm::vec3& v) const;
    void setMat3(const char* name, const glm::mat3& m) const;
    void setMat4(const char* name, const glm::mat4& m) const;
    void setVec3Array(const char* name, const glm::vec3* v, int count) const;

private:
    GLint location(const char* name) const;

    GLuint program_ = 0;
    mutable std::unordered_map<std::string, GLint> cache_;
};

}  // namespace gfx
