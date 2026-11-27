#pragma once

#include "ParticleEffect.h"

#include <glm/vec3.hpp>

#include <cstdint>
#include <vector>

namespace bm::particles {

// Durante ticks consecutivos, un ID conserva el estilo, la semilla y el tick inicial.
// La ausencia del ID termina esa vida. Si vuelve, comienza sin estela previa.
struct ParticleEmitterState {
    std::uint64_t emitterId = 0;
    ParticleStyleId styleId = 0;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f}; // Cero usa la direccion del estilo.
    int startTick = 0; // Fija la fase de emision. Puede preceder a la primera aparicion.
    std::uint64_t seed = 0;
};

struct ParticleBurst {
    std::uint64_t eventId = 0;
    ParticleStyleId styleId = 0;
    glm::vec3 position{0.0f};
    glm::vec3 direction{0.0f}; // Cero usa la direccion del estilo.
    std::uint32_t count = 0;
    std::uint64_t seed = 0;
};

struct ParticleFrame {
    // Conjunto completo. Si hay duplicados, se evalua solo la primera entrada de un ID
    // activo. Para un ID nuevo se acepta la primera entrada valida.
    std::vector<ParticleEmitterState> emitters;
    std::vector<ParticleBurst> bursts; // Cada evento solo figura en su tick de origen.
};

}
