#include "ParticleEffect.h"

namespace bm::particles::presets {

ParticleStyleCatalog makeCatalog() {
    return {
        {Fire, {
            .material = "fire", .blend = ParticleBlend::Additive,
            .particlesPerSecond = 75.0f, .minLifeTicks = 18, .maxLifeTicks = 42,
            .minSpeed = 0.7f, .maxSpeed = 2.2f, .coneAngleRadians = 0.42f,
            .acceleration = {0.0f, 0.65f, 0.0f}, .startSize = 0.22f, .endSize = 0.48f,
            .startColor = {1.0f, 0.72f, 0.24f, 0.85f}, .endColor = {0.95f, 0.16f, 0.035f, 0.0f}
        }},
        {Smoke, {
            .material = "smoke", .particlesPerSecond = 23.5f,
            .minLifeTicks = 65, .maxLifeTicks = 105,
            .minSpeed = 0.25f, .maxSpeed = 0.65f, .coneAngleRadians = 0.5f,
            .acceleration = {0.12f, 0.25f, 0.0f}, .startSize = 0.28f, .endSize = 1.35f,
            .startColor = {0.42f, 0.42f, 0.42f, 0.65f}, .endColor = {0.63f, 0.63f, 0.63f, 0.0f}
        }},
        {MagicTrail, {
            .material = "magic_trail", .blend = ParticleBlend::Additive,
            .particlesPerSecond = 145.0f, .minLifeTicks = 18, .maxLifeTicks = 41,
            .minSpeed = 0.15f, .maxSpeed = 0.75f, .coneAngleRadians = 0.7f,
            .acceleration = {0.0f, 0.2f, 0.0f}, .startSize = 0.26f, .endSize = 0.055f,
            .startColor = {0.4f, 0.8f, 1.0f, 0.9f}, .endColor = {0.5f, 0.22f, 1.0f, 0.0f}
        }},
        {Explosion, {
            .material = "spark", .blend = ParticleBlend::Additive, .emission = ParticleEmission::Radial,
            .minLifeTicks = 22, .maxLifeTicks = 58, .minSpeed = 1.5f, .maxSpeed = 5.5f,
            .acceleration = {0.0f, -4.5f, 0.0f}, .startSize = 0.24f, .endSize = 0.05f,
            .startColor = {1.0f, 0.92f, 0.48f, 1.0f}, .endColor = {1.0f, 0.25f, 0.05f, 0.0f}
        }}
    };
}

}
