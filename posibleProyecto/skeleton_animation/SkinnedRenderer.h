#pragma once

#include "SkinnedMesh.h"
#include "../MaterialLibrary.h"

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

    // La malla y su paleta se prestan hasta que termine el frame.
    void submit(const SkinnedMesh& mesh, const glm::mat4& model, std::span<const glm::mat4> boneMatrices,
                MaterialId matId, const glm::vec4& color, int kind);
    bool empty() const { return m_draws.empty(); }
    GLuint lightingProgram() const { return m_lighting.id; }
    void drawLighting(const glm::mat4& viewProj, const MaterialLibrary& matLib, const glm::vec3& cameraPosition) const;
    void drawDepth(const glm::mat4& viewProj, const glm::vec3& lightPosition, float farPlane) const;
    void clear() { m_draws.clear(); }

private:
    struct Program {
        GLuint id = 0;
        GLint model = -1, viewProj = -1, bones = -1, boneCount = -1;
    };
    struct Draw {
        const SkinnedMesh* mesh;
        glm::mat4 model;
        std::span<const glm::mat4> bones;
        MaterialId matId;
        glm::vec4 color;
        float kind;
    };

    static Program createProgram(const std::string& vertex, const char* fragment);
    void draw(const Program& program, bool lighting, const MaterialLibrary* matLib = nullptr) const;

    std::vector<Draw> m_draws;
    Program m_lighting, m_depth;
    std::size_t m_maxBones = 0;
    GLint m_locColor = -1, m_locKind = -1;
    GLint m_locLightPosition = -1, m_locFarPlane = -1;
    GLint m_locAlbedoColor = -1;
    GLint m_locEmissive = -1;
    GLint m_locRoughness = -1;
    GLint m_locMetallic = -1;
    GLint m_locOpacity = -1;
    GLint m_locCamPos = -1;
};

} // namespace bm::anim
