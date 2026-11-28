#include "SkinnedRenderer.h"
#include "../shaders.h"

#include <algorithm>
#include <stdexcept>

namespace bm::anim {
namespace {

// Las dos pasadas comparten exactamente la transformacion de posicion.
constexpr const char* VertexSource = R"GLSL(
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in ivec4 boneIds;
layout(location = 4) in vec4 boneWeights;

uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat4 uBoneMatrices[MAX_BONES];
uniform int uBoneCount;

out vec3 vWorldPos;
out vec3 vLocalPos;
#ifndef DEPTH_PASS
uniform vec4 uBaseColor;
uniform float uKind;

out vec3 vNormal;
out vec2 vUV;
out vec4 vColor;
flat out float vKind;
#endif

void main() {
    mat4 skin = mat4(0.0);
    float totalWeight = 0.0;
    for (int i = 0; i < 4; ++i) {
        int id = boneIds[i];
        float weight = boneWeights[i];
        if (id >= 0 && id < uBoneCount && weight > 0.0 &&
            !isnan(weight) && !isinf(weight)) {
            skin += weight * uBoneMatrices[id];
            totalWeight += weight;
        }
    }
    // Sin influencias validas, conserva la geometria local sin colapsarla al origen.
    if (totalWeight > 0.0) skin /= totalWeight;
    else skin = mat4(1.0);

    mat4 worldTransform = uModel * skin;
    vec4 world = worldTransform * vec4(aPosition, 1.0);
    vWorldPos = world.xyz;
    vLocalPos = aPosition;
    gl_Position = uViewProj * world;

#ifndef DEPTH_PASS
    mat3 linear = mat3(worldTransform);
    // Inversa transpuesta: tambien contempla escalas no uniformes en huesos/modelo.
    if (determinant(linear) != 0.0)
        vNormal = transpose(inverse(linear)) * aNormal;
    else
        vNormal = linear * aNormal;
    if (dot(vNormal, vNormal) == 0.0) vNormal = vec3(0.0, 1.0, 0.0);
    vUV = aUV;
    vColor = uBaseColor;
    vKind = uKind;
#endif
}
)GLSL";

} // namespace

SkinnedRenderer::~SkinnedRenderer() {
    destroy();
}

SkinnedRenderer::Program SkinnedRenderer::createProgram(const std::string& vertex, const char* fragment) {
    const GLuint id = createShaderProgram(vertex.c_str(), fragment);
    return { id, glGetUniformLocation(id, "uModel"), glGetUniformLocation(id, "uViewProj"),
        glGetUniformLocation(id, "uBoneMatrices[0]"), glGetUniformLocation(id, "uBoneCount") };
}

void SkinnedRenderer::initialize() {
    if (m_lighting.id) return;
    GLint components = 0, attributes = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &components);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &attributes);
    // 16 floats por mat4 y 48 componentes reservados para los demas uniforms.
    constexpr GLint RESERVED_COMPONENTS = 48;
    constexpr GLint MAX_BONES = 128;
    if (attributes < 5 || components < RESERVED_COMPONENTS + 16)
        throw std::runtime_error("SkinnedRenderer: limites OpenGL insuficientes");

    const auto capacity = static_cast<std::size_t>((std::min)(MAX_BONES, (components - RESERVED_COMPONENTS) / 16));
    const std::string header = "#version 330 core\nconst int MAX_BONES = " + std::to_string(capacity) + ";\n";
    m_lighting = createProgram(header + VertexSource, fragmentSrc);
    try {
        m_depth = createProgram(header + "#define DEPTH_PASS\n" + VertexSource, depthFragmentSrc);
    } catch (...) {
        destroy();
        throw;
    }
    m_maxBones = capacity;
    m_locColor = glGetUniformLocation(m_lighting.id, "uBaseColor");
    m_locKind = glGetUniformLocation(m_lighting.id, "uKind");
    m_locLightPosition = glGetUniformLocation(m_depth.id, "uLightPosition");
    m_locFarPlane = glGetUniformLocation(m_depth.id, "uFarPlane");
    m_locAlbedoColor = glGetUniformLocation(m_lighting.id, "uAlbedoColor");
    m_locEmissive = glGetUniformLocation(m_lighting.id, "uEmissive");
    m_locRoughness = glGetUniformLocation(m_lighting.id, "uRoughness");
    m_locMetallic = glGetUniformLocation(m_lighting.id, "uMetallic");
    m_locOpacity = glGetUniformLocation(m_lighting.id, "uOpacity");
    m_locCamPos = glGetUniformLocation(m_lighting.id, "uCamPos");

    glUseProgram(m_lighting.id);
    glUniform1i(glGetUniformLocation(m_lighting.id, "uAlbedo"), 0);
    glUseProgram(0);
}

void SkinnedRenderer::submit(const SkinnedMesh& mesh, const glm::mat4& model,
    std::span<const glm::mat4> boneMatrices, MaterialId matId, const glm::vec4& color, int kind) {
    if (boneMatrices.size() != mesh.boneCount() || boneMatrices.size() > m_maxBones)
        throw std::invalid_argument("SkinnedRenderer: paleta de huesos invalida");
    m_draws.push_back({ &mesh, model, boneMatrices, matId, mesh.baseColor() * color, static_cast<float>(kind) });
}

void SkinnedRenderer::drawLighting(const glm::mat4& viewProj, const MaterialLibrary& matLib, const glm::vec3& cameraPosition) const {
    if (empty()) return;

    glUseProgram(m_lighting.id);
    glUniformMatrix4fv(m_lighting.viewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform3fv(m_locCamPos, 1, glm::value_ptr(cameraPosition));   // comparte shader con el Renderer: sin esto los brillos salian desde (0,0,0)
    glActiveTexture(GL_TEXTURE0);
    draw(m_lighting, true, &matLib);
}

void SkinnedRenderer::drawDepth(const glm::mat4& viewProj, const glm::vec3& lightPosition, float farPlane) const {
    if (empty()) return;

    glUseProgram(m_depth.id);
    glUniformMatrix4fv(m_depth.viewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform3fv(m_locLightPosition, 1, glm::value_ptr(lightPosition));
    glUniform1f(m_locFarPlane, farPlane);
    draw(m_depth, false);
}

void SkinnedRenderer::draw(const Program& program, bool lighting, const MaterialLibrary* matLib) const {
    for (const Draw& part : m_draws) {
        glUniformMatrix4fv(program.model, 1, GL_FALSE, glm::value_ptr(part.model));
        glUniform1i(program.boneCount, static_cast<GLint>(part.bones.size()));
        if (!part.bones.empty())
            glUniformMatrix4fv(program.bones, static_cast<GLsizei>(part.bones.size()), GL_FALSE,
                glm::value_ptr(part.bones.front()));
        if (lighting) {
            const Material& mat = matLib->get(part.matId);
            glUniform4fv(m_locColor, 1, glm::value_ptr(part.color));

            glUniform4fv(m_locAlbedoColor, 1, glm::value_ptr(mat.albedoColor));

            glUniform3fv(m_locEmissive, 1, glm::value_ptr(mat.emissive));

            glUniform1f(m_locRoughness, mat.roughness);
            glUniform1f(m_locMetallic, mat.metallic);
            glUniform1f(m_locOpacity, mat.opacity);

            glActiveTexture(GL_TEXTURE0);

            glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
        }
        part.mesh->draw();
    }
}

void SkinnedRenderer::destroy() {
    clear();
    if (m_lighting.id) glDeleteProgram(m_lighting.id);
    if (m_depth.id) glDeleteProgram(m_depth.id);
    m_lighting = {};
    m_depth = {};
    m_maxBones = 0;
}

} // namespace bm::anim
