#pragma once

#include "Particle.h"
#include "ParticleEffect.h"
#include "ParticleFrame.h"
#include "ParticlePlayback.h"
#include "ParticleRendererGL.h"
#include "ParticleSystem.h"

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace bm::particles {

class ParticleModule {
public:
    explicit ParticleModule(std::size_t capacity = ParticleSystem::DefaultCapacity);

    // Inicializacion y dibujo requieren el contexto OpenGL activo y GLEW inicializado.
    void initialize(ParticleStyleCatalog styles, ParticleMaterialTable materials = {});
    void setFrames(std::vector<ParticleFrame> frames, int ticksPerSecond);
    void seek(int tick);
    void render(const glm::mat4& view, const glm::mat4& projection);
    // Liberacion explicita con el mismo contexto activo, antes de destruirlo.
    void shutdown();

    std::span<const Particle> particles() const { return system_.particles(); }

private:
    ParticleSystem system_;
    ParticlePlayback playback_;
    ParticleRendererGL renderer_;
};

}
