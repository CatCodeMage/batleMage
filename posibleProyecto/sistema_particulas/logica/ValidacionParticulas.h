#pragma once

#include "../ParticleEffect.h"

#include <glm/gtc/constants.hpp>

#include <cmath>

namespace bm::particles::detalle {

inline bool esFinito(const glm::vec3& valor) {
    return std::isfinite(valor.x) && std::isfinite(valor.y) && std::isfinite(valor.z);
}

inline bool estiloValido(const ParticleStyle& estilo) {
    const auto noNegativo = [](float valor) { return std::isfinite(valor) && valor >= 0.0f; };
    const auto colorValido = [](const glm::vec4& color) {
        return esFinito(glm::vec3(color)) && color.a >= 0.0f && color.a <= 1.0f;
    };

    return (estilo.blend == ParticleBlend::Alpha || estilo.blend == ParticleBlend::Additive) &&
        (estilo.emission == ParticleEmission::Directional || estilo.emission == ParticleEmission::Radial) &&
        noNegativo(estilo.particlesPerSecond) &&
        estilo.minLifeTicks > 0 && estilo.maxLifeTicks >= estilo.minLifeTicks &&
        noNegativo(estilo.minSpeed) && noNegativo(estilo.maxSpeed) && estilo.maxSpeed >= estilo.minSpeed &&
        noNegativo(estilo.coneAngleRadians) && estilo.coneAngleRadians <= glm::pi<float>() &&
        esFinito(estilo.direction) && esFinito(estilo.acceleration) &&
        noNegativo(estilo.startSize) && noNegativo(estilo.endSize) &&
        colorValido(estilo.startColor) && colorValido(estilo.endColor);
}

}
