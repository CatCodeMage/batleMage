#include "SkinnedRenderer.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <stdexcept>
#include <string>

namespace bm::anim {
namespace {

// El tamano de uBoneMatrices se antepone al compilar, segun los limites reales.
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

out vec3 vNormal;
out vec2 vUV;

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
    gl_Position = uViewProj * worldTransform * vec4(aPosition, 1.0);
    mat3 linear = mat3(worldTransform);
    // Inversa transpuesta: tambien contempla escalas no uniformes en huesos/modelo.
    if (abs(determinant(linear)) > 1e-8)
        vNormal = transpose(inverse(linear)) * aNormal;
    else
        vNormal = linear * aNormal;
    vUV = aUV;
}
)GLSL";

constexpr const char* FragmentSource = R"GLSL(#version 330 core
in vec3 vNormal;
in vec2 vUV;
uniform sampler2D uAlbedo;
uniform int uHasAlbedo;
uniform vec4 uBaseColor;
out vec4 FragColor;

void main() {
    vec4 base = uBaseColor;
    if (uHasAlbedo != 0) base *= texture(uAlbedo, vUV);
    vec3 normal = vec3(0.0, 1.0, 0.0);
    if (dot(vNormal, vNormal) > 1e-12) normal = normalize(vNormal);
    // La misma luz hemisferica de referencia del motor. Luces/sombras: futura integracion.
    vec3 sky = vec3(0.45, 0.55, 0.75);
    vec3 ground = vec3(0.12, 0.10, 0.08);
    vec3 light = mix(ground, sky, normal.y * 0.5 + 0.5) * 0.6;
    FragColor = vec4(base.rgb * light, base.a);
}
)GLSL";

GLuint compileStage(GLenum type, const char* source) {
    const GLuint shader = glCreateShader(type);
    if (!shader) throw std::runtime_error("SkinnedRenderer: no se pudo crear el shader");
    try {
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        GLint compiled = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
        if (compiled != GL_TRUE) {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>((std::max)(length, 1)), '\0');
            glGetShaderInfoLog(shader, static_cast<GLsizei>(log.size()), nullptr, log.data());
            throw std::runtime_error("SkinnedRenderer: error compilando shader: " + log);
        }
    } catch (...) {
        glDeleteShader(shader);
        throw;
    }
    return shader;
}

GLuint createProgram(std::size_t maxBones) {
    const std::string vertex = "#version 330 core\nconst int MAX_BONES = " +
                               std::to_string(maxBones) + ";\n" + VertexSource;
    GLuint vs = 0, fs = 0, program = 0;
    try {
        vs = compileStage(GL_VERTEX_SHADER, vertex.c_str());
        fs = compileStage(GL_FRAGMENT_SHADER, FragmentSource);
        program = glCreateProgram();
        if (!program) throw std::runtime_error("SkinnedRenderer: no se pudo crear el programa");
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (linked != GL_TRUE) {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            std::string log(static_cast<std::size_t>((std::max)(length, 1)), '\0');
            glGetProgramInfoLog(program, static_cast<GLsizei>(log.size()), nullptr, log.data());
            throw std::runtime_error("SkinnedRenderer: error enlazando shader: " + log);
        }
    } catch (...) {
        if (program) glDeleteProgram(program);
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        throw;
    }
    glDetachShader(program, vs);
    glDetachShader(program, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

} // namespace

SkinnedRenderer::~SkinnedRenderer() {
    destroy();
}

void SkinnedRenderer::initialize() {
    if (m_program) return;
    GLint components = 0, attributes = 0;
    glGetIntegerv(GL_MAX_VERTEX_UNIFORM_COMPONENTS, &components);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &attributes);
    // 16 floats por mat4. Reservamos 48 para modelo, viewProj, contador y margen.
    constexpr GLint RESERVED_COMPONENTS = 48;
    constexpr GLint MAX_BONES = 64;
    if (attributes < 5 || components < RESERVED_COMPONENTS + 16) {
        throw std::runtime_error("SkinnedRenderer: limites OpenGL insuficientes");
    }
    const auto capacity = static_cast<std::size_t>((std::min)(MAX_BONES, (components - RESERVED_COMPONENTS) / 16));
    m_program = createProgram(capacity);
    m_maxBones = capacity;
    m_locModel = glGetUniformLocation(m_program, "uModel");
    m_locViewProj = glGetUniformLocation(m_program, "uViewProj");
    m_locBones = glGetUniformLocation(m_program, "uBoneMatrices[0]");
    m_locBoneCount = glGetUniformLocation(m_program, "uBoneCount");
    m_locColor = glGetUniformLocation(m_program, "uBaseColor");
    m_locAlbedo = glGetUniformLocation(m_program, "uAlbedo");
    m_locHasAlbedo = glGetUniformLocation(m_program, "uHasAlbedo");
}

void SkinnedRenderer::draw(const SkinnedMesh& mesh, const glm::mat4& model,
                           const glm::mat4& viewProj, std::span<const glm::mat4> boneMatrices,
                           GLuint albedoTexture, const glm::vec4& color) const {
    if (!m_program) throw std::logic_error("SkinnedRenderer: falta initialize()");
    if (boneMatrices.size() != mesh.boneCount()) {
        throw std::invalid_argument("SkinnedRenderer: la paleta no coincide con la submalla");
    }
    if (boneMatrices.size() > m_maxBones) {
        throw std::invalid_argument("SkinnedRenderer: la submalla supera el limite de huesos (" +
                                    std::to_string(m_maxBones) + ")");
    }

    GLint previousProgram = 0, previousActiveTexture = 0, previousTexture = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &previousProgram);
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    glUseProgram(m_program);
    glUniformMatrix4fv(m_locModel, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(m_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform1i(m_locBoneCount, static_cast<GLint>(boneMatrices.size()));
    if (!boneMatrices.empty()) {
        glUniformMatrix4fv(m_locBones, static_cast<GLsizei>(boneMatrices.size()), GL_FALSE,
                           glm::value_ptr(boneMatrices.front()));
    }
    const glm::vec4 baseColor = mesh.baseColor() * color;
    glUniform4fv(m_locColor, 1, glm::value_ptr(baseColor));
    glUniform1i(m_locAlbedo, 0);
    glUniform1i(m_locHasAlbedo, albedoTexture != 0);
    glBindTexture(GL_TEXTURE_2D, albedoTexture);
    mesh.draw();

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glActiveTexture(static_cast<GLenum>(previousActiveTexture));
    glUseProgram(static_cast<GLuint>(previousProgram));
}

void SkinnedRenderer::destroy() {
    if (m_program) glDeleteProgram(m_program);
    m_program = 0;
    m_maxBones = 0;
    m_locModel = m_locViewProj = m_locBones = -1;
    m_locBoneCount = m_locColor = m_locAlbedo = m_locHasAlbedo = -1;
}

} // namespace bm::anim
