#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>

namespace bm::particles {

using ParticleStyleId = std::uint32_t;

enum class ParticleBlend { Alpha, Additive };
enum class ParticleEmission { Directional, Radial };

struct ParticleStyle {
    std::string material;
    ParticleBlend blend = ParticleBlend::Alpha;
    ParticleEmission emission = ParticleEmission::Directional;
    float particlesPerSecond = 0.0f;
    int minLifeTicks = 1;
    int maxLifeTicks = 1;
    float minSpeed = 0.0f;
    float maxSpeed = 0.0f;
    glm::vec3 direction{0.0f, 1.0f, 0.0f};
    float coneAngleRadians = 0.0f; // Semiangulo de apertura.
    glm::vec3 acceleration{0.0f};
    float startSize = 1.0f;
    float endSize = 1.0f;
    glm::vec4 startColor{1.0f};
    glm::vec4 endColor{1.0f, 1.0f, 1.0f, 0.0f};
};

// La clave del catalogo identifica el estilo en el replay.
using ParticleStyleCatalog = std::unordered_map<ParticleStyleId, ParticleStyle>;

namespace presets {
inline constexpr ParticleStyleId Fire = 1;
inline constexpr ParticleStyleId Smoke = 2;
inline constexpr ParticleStyleId MagicTrail = 3;
inline constexpr ParticleStyleId Explosion = 4;

ParticleStyleCatalog makeCatalog();
}

}
