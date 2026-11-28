#pragma once

#include "Particle.h"
#include "ParticleEffect.h"
#include "ParticleFrame.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <vector>

namespace bm::particles {

class ParticleSystem {
public:
    static constexpr std::size_t DefaultCapacity = 5000;

    explicit ParticleSystem(std::size_t capacity = DefaultCapacity);
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    // Configure los estilos antes de entregar los fotogramas a ParticlePlayback.
    void setStyles(ParticleStyleCatalog styles);
    void reset();
    const ParticleStyleCatalog& styles() const { return styles_; }
    std::span<const Particle> particles() const { return particles_; }

private:
    friend class ParticlePlayback;

    struct ActiveEmitter {
        glm::vec3 position;
        ParticleStyleId styleId;
        std::uint64_t seed;
        int startTick;
        int lastTick;
    };

    void step(const ParticleFrame& frame, int tick, int ticksPerSecond);
    void updateParticles(float fixedStep);
    void spawn(ParticleStyleId styleId, const ParticleStyle& style, const glm::vec3& from,
               const glm::vec3& to, const glm::vec3& direction, double count,
               std::uint64_t emissionKey);

    std::size_t capacity_;
    ParticleStyleCatalog styles_;
    std::vector<Particle> particles_;
    std::unordered_map<std::uint64_t, ActiveEmitter> activeEmitters_;
};

}
