#include "ParticleRendererGL.h"

#include "../shaders.h"

#include <glm/gtc/type_ptr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace bm::particles {
namespace {

constexpr char vertexSource[] = R"GLSL(#version 330 core
layout(location = 0) in vec2 aCorner;
layout(location = 1) in vec4 iPositionAndSize;
layout(location = 2) in vec4 iColor;
layout(location = 3) in float iRotation;

uniform mat4 uView;
uniform mat4 uProjection;
out vec2 vUV;
out vec4 vColor;

void main() {
    float c = cos(iRotation);
    float s = sin(iRotation);
    vec2 corner = mat2(c, s, -s, c) * aCorner * iPositionAndSize.w;
    // Desplazar el centro en XY de vista orienta el quad hacia la camara.
    vec4 center = uView * vec4(iPositionAndSize.xyz, 1.0);
    gl_Position = uProjection * (center + vec4(corner, 0.0, 0.0));
    vUV = aCorner + vec2(0.5);
    vColor = iColor;
}
)GLSL";

constexpr char fragmentSource[] = R"GLSL(#version 330 core
uniform sampler2D uTexture;
in vec2 vUV;
in vec4 vColor;
layout(location = 0) out vec4 fragmentColor;

void main() {
    fragmentColor = texture(uTexture, vUV) * vColor;
}
)GLSL";

constexpr GLfloat quad[] = {-0.5f, -0.5f, 0.5f, -0.5f, -0.5f, 0.5f, 0.5f, 0.5f};

struct VertexBindings {
    GLint vao;
    GLint arrayBuffer;

    VertexBindings() {
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &arrayBuffer);
    }

    ~VertexBindings() {
        glBindVertexArray(static_cast<GLuint>(vao));
        glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(arrayBuffer));
    }
};

struct DrawState {
    VertexBindings vertexBindings;
    GLint program;
    GLint activeTexture;
    GLint texture;
    GLint sampler;
    GLboolean depthWrite;
    GLboolean depthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean cullFace = glIsEnabled(GL_CULL_FACE);
    GLboolean blendEnabled = glIsEnabledi(GL_BLEND, 0);
    GLint sourceRgb, destRgb, sourceAlpha, destAlpha, equationRgb, equationAlpha;

    DrawState() {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depthWrite);

        glGetIntegerv(GL_BLEND_SRC_RGB, &sourceRgb);
        glGetIntegerv(GL_BLEND_DST_RGB, &destRgb);
        glGetIntegerv(GL_BLEND_SRC_ALPHA, &sourceAlpha);
        glGetIntegerv(GL_BLEND_DST_ALPHA, &destAlpha);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, &equationRgb);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &equationAlpha);

        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
        glGetIntegerv(GL_SAMPLER_BINDING, &sampler);
        glBindSampler(0, 0);
        // Solo la salida 0 cambia de activacion. Los factores son globales en 3.3.
        glEnablei(GL_BLEND, 0);
        glBlendEquation(GL_FUNC_ADD);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
    }

    ~DrawState() {
        glBlendFuncSeparate(sourceRgb, destRgb, sourceAlpha, destAlpha);
        glBlendEquationSeparate(equationRgb, equationAlpha);
        if (!blendEnabled) glDisablei(GL_BLEND, 0);
        if (!depthTest) glDisable(GL_DEPTH_TEST);
        if (cullFace) glEnable(GL_CULL_FACE);
        glDepthMask(depthWrite);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(texture));
        glBindSampler(0, static_cast<GLuint>(sampler));
        glActiveTexture(static_cast<GLenum>(activeTexture));
        glUseProgram(static_cast<GLuint>(program));
    }
};

GLuint makeFallbackTexture() {
    constexpr int size = 32;
    constexpr int channels = 4;
    std::array<GLubyte, size * size * channels> pixels;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const float dx = (x + 0.5f) * (2.0f / size) - 1.0f;
            const float dy = (y + 0.5f) * (2.0f / size) - 1.0f;
            const float coverage = std::max(0.0f, 1.0f - dx * dx - dy * dy);
            const int offset = (y * size + x) * channels;
            pixels[offset] = pixels[offset + 1] = pixels[offset + 2] = 255;
            pixels[offset + 3] = static_cast<GLubyte>(255.0f * coverage * coverage + 0.5f);
        }
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    if (!texture) return 0;

    GLint previousTexture, unpackBuffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    // Las filas RGBA de 128 bytes cumplen cualquier alineacion de unpack.
    constexpr GLenum unpackSettings[] = {GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS};
    GLint previousPacking[std::size(unpackSettings)];
    for (std::size_t i = 0; i < std::size(unpackSettings); ++i) {
        glGetIntegerv(unpackSettings[i], &previousPacking[i]);
        glPixelStorei(unpackSettings[i], 0);
    }

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    GLint allocatedWidth = 0;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &allocatedWidth);

    for (std::size_t i = 0; i < std::size(unpackSettings); ++i) {
        glPixelStorei(unpackSettings[i], previousPacking[i]);
    }
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(unpackBuffer));
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    if (allocatedWidth != size) {
        glDeleteTextures(1, &texture);
        return 0;
    }
    return texture;
}

}

void ParticleRendererGL::initialize(ParticleMaterialTable materials) {
    if (!glGetString(GL_VERSION) || !GLEW_VERSION_3_3) {
        throw std::runtime_error("Las particulas requieren un contexto OpenGL 3.3 y GLEW inicializado");
    }
    if (program_) {
        materials_ = std::move(materials);
        return;
    }

    try {
        program_ = createShaderProgram(vertexSource, fragmentSource);
        viewLocation_ = glGetUniformLocation(program_, "uView");
        projectionLocation_ = glGetUniformLocation(program_, "uProjection");
        // El sampler usa la unidad 0, el valor inicial de los uniforms tras enlazar.

        VertexBindings restoreBindings;
        glGenVertexArrays(1, &vao_);
        glGenBuffers(1, &quadVbo_);
        glGenBuffers(1, &instanceVbo_);
        fallbackTexture_ = makeFallbackTexture();
        if (!vao_ || !quadVbo_ || !instanceVbo_ || !fallbackTexture_) {
            throw std::runtime_error("No se pudieron crear los recursos OpenGL de particulas");
        }

        glBindVertexArray(vao_);
        glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        // Un ID generado no garantiza que se haya podido reservar el almacenamiento.
        GLint quadBytes = 0;
        glGetBufferParameteriv(GL_ARRAY_BUFFER, GL_BUFFER_SIZE, &quadBytes);
        if (quadBytes != sizeof(quad)) throw std::runtime_error("No se pudo cargar el quad de particulas");
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
        glEnableVertexAttribArray(0);
        for (GLuint location = 1; location <= 3; ++location) {
            glEnableVertexAttribArray(location);
            glVertexAttribDivisor(location, 1);
        }
        materials_ = std::move(materials);
    } catch (...) {
        shutdown();
        throw;
    }
}

void ParticleRendererGL::render(std::span<const Particle> particles, const ParticleStyleCatalog& styles,
                                const glm::mat4& view, const glm::mat4& projection) {
    static_assert(std::is_standard_layout_v<InstanceData>);
    if (!program_ || particles.empty()) return;
    if (!glGetString(GL_VERSION)) {
        throw std::runtime_error("El contexto OpenGL debe estar activo para dibujar particulas");
    }
    constexpr auto maxInstances = (std::numeric_limits<GLsizei>::max)();
    constexpr auto maxBytes = (std::numeric_limits<GLsizeiptr>::max)();
    if (particles.size() > maxInstances || particles.size() > maxBytes / sizeof(InstanceData)) {
        throw std::length_error("Demasiadas instancias de particulas para OpenGL");
    }

    drawItems_.clear();
    drawItems_.reserve(particles.size());
    for (const auto& particle : particles) {
        if (particle.color.a <= 0.0f || particle.size <= 0.0f) continue;
        const auto style = styles.find(particle.styleId);
        if (style == styles.end()) continue;
        const float depth = -(view * glm::vec4(particle.position, 1.0f)).z;
        if (!std::isfinite(depth)) continue;
        const auto material = materials_.find(style->second.material);
        const GLuint texture = material != materials_.end() && material->second ? material->second : fallbackTexture_;
        drawItems_.push_back({&particle, depth, style->second.blend, texture});
    }
    if (drawItems_.empty()) return;

    // Orden global: el humo cercano tambien debe atenuar particulas aditivas lejanas.
    std::sort(drawItems_.begin(), drawItems_.end(), [](const DrawItem& a, const DrawItem& b) {
        if (a.depth == b.depth) return a.particle < b.particle;
        return a.depth > b.depth;
    });

    instances_.clear();
    instances_.reserve(drawItems_.size());
    for (const auto& item : drawItems_) {
        const auto& particle = *item.particle;
        instances_.push_back({glm::vec4(particle.position, particle.size), particle.color, particle.rotation});
    }

    const DrawState restoreState;
    glUseProgram(program_);
    glUniformMatrix4fv(viewLocation_, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projectionLocation_, 1, GL_FALSE, glm::value_ptr(projection));
    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances_.size() * sizeof(InstanceData)),
                 instances_.data(), GL_STREAM_DRAW);

    for (std::size_t first = 0; first < drawItems_.size();) {
        const auto& item = drawItems_[first];
        std::size_t end = first + 1;
        while (end < drawItems_.size() && drawItems_[end].texture == item.texture && drawItems_[end].blend == item.blend) ++end;

        const GLenum destination = item.blend == ParticleBlend::Additive ? GL_ONE : GL_ONE_MINUS_SRC_ALPHA;
        glBlendFunc(GL_SRC_ALPHA, destination);
        glBindTexture(GL_TEXTURE_2D, item.texture);

        // OpenGL 3.3 selecciona el tramo de instancias con el offset de los atributos.
        const std::size_t offset = first * sizeof(InstanceData);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              reinterpret_cast<const void*>(offset + offsetof(InstanceData, positionAndSize)));
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              reinterpret_cast<const void*>(offset + offsetof(InstanceData, color)));
        glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
                              reinterpret_cast<const void*>(offset + offsetof(InstanceData, rotation)));
        glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, static_cast<GLsizei>(end - first));
        first = end;
    }
}

void ParticleRendererGL::shutdown() {
    if (!program_) return;
    if (!glGetString(GL_VERSION)) {
        throw std::runtime_error("El contexto OpenGL debe estar activo para liberar las particulas");
    }

    const GLuint buffers[] = {quadVbo_, instanceVbo_};
    glDeleteVertexArrays(1, &vao_);
    glDeleteBuffers(2, buffers);
    glDeleteTextures(1, &fallbackTexture_);
    glDeleteProgram(program_);
    fallbackTexture_ = vao_ = quadVbo_ = instanceVbo_ = program_ = 0;
    viewLocation_ = projectionLocation_ = -1;
    materials_.clear();
    drawItems_.clear();
    instances_.clear();
}

}
