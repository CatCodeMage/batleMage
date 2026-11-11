#pragma once

#include "SkinnedMesh.h"

#include <span>

namespace bm::anim {

class SkinnedRenderer {
public:
    SkinnedRenderer() = default;
    ~SkinnedRenderer();
    SkinnedRenderer(const SkinnedRenderer&) = delete;
    SkinnedRenderer& operator=(const SkinnedRenderer&) = delete;

    // Llamar tras crear el contexto OpenGL 3.3 e inicializar GLEW.
    // Lanza runtime_error con el log si falla el shader o el hardware es insuficiente.
    void initialize();
    void destroy(); // Antes de cerrar el contexto; repetible.
    std::size_t maxBones() const { return m_maxBones; }

    // model = characterModel * animator.meshTransform(meshIndex).
    // La paleta debe corresponder a ESTA submalla (vacia para una malla rigida).
    // albedoTexture es prestada; 0 utiliza solo el color del material y el tinte.
    // El llamador configura framebuffer, viewport, profundidad, culling y blending.
    // Restaura programa, VAO, unidad activa y textura 2D de la unidad 0.
    void draw(const SkinnedMesh& mesh, const glm::mat4& model,
              const glm::mat4& viewProj, std::span<const glm::mat4> boneMatrices,
              GLuint albedoTexture = 0, const glm::vec4& color = glm::vec4(1.0f)) const;

private:
    GLuint m_program = 0;
    std::size_t m_maxBones = 0;
    GLint m_locModel = -1, m_locViewProj = -1, m_locBones = -1;
    GLint m_locBoneCount = -1, m_locColor = -1, m_locAlbedo = -1, m_locHasAlbedo = -1;
};

} // namespace bm::anim
