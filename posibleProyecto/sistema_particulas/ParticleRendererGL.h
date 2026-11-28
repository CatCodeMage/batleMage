#pragma once

#include <GL/glew.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Particle.h"
#include "ParticleEffect.h"

#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace bm::particles {

// Texturas prestadas de TextureLibrary u otro propietario. Un valor cero usa el respaldo.
using ParticleMaterialTable = std::unordered_map<std::string, GLuint>;

class ParticleRendererGL {
public:
    ParticleRendererGL() = default;
    ~ParticleRendererGL() = default;
    ParticleRendererGL(const ParticleRendererGL&) = delete;
    ParticleRendererGL& operator=(const ParticleRendererGL&) = delete;

    // Requiere GLEW inicializado y el mismo contexto activo para inicializar, dibujar y liberar.
    // La segunda inicializacion solo actualiza la tabla de materiales.
    void initialize(ParticleMaterialTable materials);
    // Pase de color 0 con factores y ecuaciones de blending comunes a todas las salidas (OpenGL 3.3).
    void render(std::span<const Particle> particles, const ParticleStyleCatalog& styles,
                const glm::mat4& view, const glm::mat4& projection);
    // Libera los recursos propios una sola vez. Llame antes de destruir el contexto:
    // el destructor es solo CPU. Si hay recursos y falta el contexto activo, lanza una excepcion.
    void shutdown();

private:
    struct DrawItem {
        const Particle* particle;
        float depth;
        ParticleBlend blend;
        GLuint texture;
    };

    struct InstanceData {
        glm::vec4 positionAndSize;
        glm::vec4 color;
        float rotation;
    };

    ParticleMaterialTable materials_;
    std::vector<DrawItem> drawItems_;
    std::vector<InstanceData> instances_;
    GLuint vao_ = 0;
    GLuint quadVbo_ = 0;
    GLuint instanceVbo_ = 0;
    GLuint program_ = 0;
    GLuint fallbackTexture_ = 0;
    GLint viewLocation_ = -1;
    GLint projectionLocation_ = -1;
};

}
