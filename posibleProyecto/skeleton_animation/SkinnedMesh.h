#pragma once

#include <GL/glew.h>
#include "AssimpModelLoader.h"

namespace bm::anim {

// Crear, dibujar y destruir con un contexto OpenGL 3.3 y GLEW activos.
class SkinnedMesh {
public:
    SkinnedMesh() = default;
    explicit SkinnedMesh(const ImportedMesh& mesh);
    ~SkinnedMesh();

    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;
    SkinnedMesh(SkinnedMesh&& other) noexcept;
    SkinnedMesh& operator=(SkinnedMesh&& other) noexcept;

    void create(const ImportedMesh& mesh);
    void draw() const;
    // Llamar antes de cerrar el contexto; se puede repetir sin efectos.
    void destroy();
    std::size_t boneCount() const { return m_boneCount; }
    const glm::vec4& baseColor() const { return m_baseColor; }

private:
    GLuint m_vao = 0, m_vbo = 0, m_ebo = 0;
    GLsizei m_indexCount = 0;
    std::size_t m_boneCount = 0;
    glm::vec4 m_baseColor{1.0f};
};

} // namespace bm::anim
