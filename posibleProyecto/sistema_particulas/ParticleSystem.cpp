#include "ParticleSystem.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>

namespace bm::particles {
namespace {

bool isFinite(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool isValidColor(const glm::vec4& color) {
    return isFinite(glm::vec3(color)) && color.a >= 0.0f && color.a <= 1.0f;
}

bool isNonnegative(float value) {
    return std::isfinite(value) && value >= 0.0f;
}

bool isValidStyle(const ParticleStyle& style) {
    return isNonnegative(style.particlesPerSecond) &&
        style.minLifeTicks > 0 && style.maxLifeTicks >= style.minLifeTicks &&
        isNonnegative(style.minSpeed) && isNonnegative(style.maxSpeed) && style.maxSpeed >= style.minSpeed &&
        isNonnegative(style.coneAngleRadians) && style.coneAngleRadians <= glm::pi<float>() &&
        isFinite(style.direction) && isFinite(style.acceleration) &&
        isNonnegative(style.startSize) && isNonnegative(style.endSize) &&
        isValidColor(style.startColor) && isValidColor(style.endColor);
}

// Mezcla SplitMix64 con enteros de ancho fijo para evitar un estado aleatorio global.
std::uint64_t mixBits(std::uint64_t value) {
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

std::uint64_t makeEmissionKey(std::uint64_t seed, std::uint64_t id, int tick, bool burst) {
    const auto tickAndKind = (static_cast<std::uint64_t>(tick) << 1) | static_cast<std::uint64_t>(burst);
    return mixBits(mixBits(seed) ^ id ^ 0x9e3779b97f4a7c15ULL) ^ mixBits(tickAndKind);
}

struct Random {
    std::uint64_t state;

    std::uint64_t next() {
        state += 0x9e3779b97f4a7c15ULL;
        return mixBits(state);
    }

    float unit() {
        return static_cast<float>(next() >> 40) * (1.0f / 16777216.0f);
    }
};

glm::vec3 unitDirection(const glm::vec3& direction, const glm::vec3& fallback) {
    if (!isFinite(direction)) return fallback;
    const float scale = std::max({std::abs(direction.x), std::abs(direction.y), std::abs(direction.z)});
    if (scale == 0.0f) return fallback;
    return glm::normalize(direction / scale);
}

}

ParticleSystem::ParticleSystem(std::size_t capacity) : capacity_(capacity) {
    particles_.reserve(capacity_);
}

void ParticleSystem::setStyles(ParticleStyleCatalog styles) {
    for (const auto& [id, style] : styles) {
        if (!isValidStyle(style)) throw std::invalid_argument("Estilo de particulas invalido: " + std::to_string(id));
    }
    styles_ = std::move(styles);
    reset();
}

void ParticleSystem::reset() {
    particles_.clear();
    activeEmitters_.clear();
}

void ParticleSystem::updateParticles(float fixedStep) {
    // Compactar en orden conserva el orden de nacimiento y deja libres las plazas al final.
    std::size_t livingCount = 0;
    for (std::size_t i = 0; i < particles_.size(); ++i) {
        Particle& particle = particles_[i];
        ++particle.ageTicks;
        // Una vida de un tick se ve solo al nacer y sale antes de interpolar.
        if (particle.ageTicks >= particle.lifeTicks) continue;

        const auto& style = styles_.at(particle.styleId);
        particle.velocity += style.acceleration * fixedStep;
        particle.position += particle.velocity * fixedStep;
        // La ultima edad visible alcanza el color y el tamano finales.
        const float progress = static_cast<float>(particle.ageTicks) / (particle.lifeTicks - 1);
        particle.color = glm::mix(style.startColor, style.endColor, progress);
        particle.size = glm::mix(style.startSize, style.endSize, progress);
        if (livingCount != i) particles_[livingCount] = particle;
        ++livingCount;
    }
    particles_.resize(livingCount);
}

void ParticleSystem::step(const ParticleFrame& frame, int tick, int ticksPerSecond) {
    // Primero envejecen las particulas vivas. Los nacimientos de este tick tienen edad cero.
    updateParticles(1.0f / static_cast<float>(ticksPerSecond));

    // Las plazas libres van primero a los bursts y despues a las emisiones continuas.
    // Dentro de cada grupo se respeta el orden del fotograma.
    for (const auto& burst : frame.bursts) {
        const auto style = styles_.find(burst.styleId);
        if (style == styles_.end() || !isFinite(burst.position)) continue;
        spawn(burst.styleId, style->second, burst.position, burst.position, burst.direction,
              burst.count, makeEmissionKey(burst.seed, burst.eventId, tick, true));
    }

    for (const auto& emitter : frame.emitters) {
        const auto active = activeEmitters_.find(emitter.emitterId);
        if (active != activeEmitters_.end()) {
            auto& previous = active->second;
            if (previous.lastTick == tick) continue;
            previous.lastTick = tick;
            // Ignorar el cambio conserva la identidad y la ultima posicion aceptada.
            if (previous.styleId != emitter.styleId || previous.seed != emitter.seed ||
                previous.startTick != emitter.startTick) continue;
        }

        const auto style = styles_.find(emitter.styleId);
        if (style == styles_.end() || emitter.startTick < 0 || emitter.startTick > tick || !isFinite(emitter.position)) continue;

        glm::vec3 previousPosition = emitter.position;
        if (active == activeEmitters_.end()) {
            activeEmitters_.emplace(emitter.emitterId, ActiveEmitter{
                emitter.position, emitter.styleId, emitter.seed, emitter.startTick, tick
            });
        } else {
            previousPosition = active->second.position;
            active->second.position = emitter.position;
        }

        const double relativeTick = tick - emitter.startTick;
        const double rate = style->second.particlesPerSecond;
        const double count = std::floor((relativeTick + 1.0) * rate / ticksPerSecond) -
            std::floor(relativeTick * rate / ticksPerSecond);
        spawn(emitter.styleId, style->second, previousPosition, emitter.position, emitter.direction,
              count, makeEmissionKey(emitter.seed, emitter.emitterId, tick, false));
    }
    std::erase_if(activeEmitters_, [tick](const auto& entry) { return entry.second.lastTick != tick; });
}

void ParticleSystem::spawn(ParticleStyleId styleId, const ParticleStyle& style, const glm::vec3& from,
                           const glm::vec3& to, const glm::vec3& direction, double count,
                           std::uint64_t emissionKey) {
    const std::size_t available = capacity_ - particles_.size();
    if (available == 0 || count <= 0.0) return;
    // Se descartan los nacimientos que no caben sin expulsar particulas vivas.
    const std::size_t births = count >= static_cast<double>(available) ? available : static_cast<std::size_t>(count);
    const bool radial = style.emission == ParticleEmission::Radial;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    const glm::vec3 axis = radial ? up : unitDirection(direction, unitDirection(style.direction, up));
    const glm::vec3 reference = std::abs(axis.y) < 0.99f ? up : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(reference, axis));
    const glm::vec3 bitangent = glm::cross(axis, tangent);
    const float minCosine = radial ? -1.0f : std::cos(style.coneAngleRadians);
    const auto lifeRange = static_cast<std::uint64_t>(style.maxLifeTicks - style.minLifeTicks) + 1;

    for (std::size_t i = 0; i < births; ++i) {
        Random random{mixBits(emissionKey ^ static_cast<std::uint64_t>(i))};
        // Muestreo uniforme del angulo solido del cono o de la esfera completa.
        const float cosine = 1.0f - (1.0f - minCosine) * random.unit();
        const float sine = std::sqrt(glm::max(0.0f, 1.0f - cosine * cosine));
        const float azimuth = glm::two_pi<float>() * random.unit();
        const glm::vec3 heading = cosine * axis + sine * (std::cos(azimuth) * tangent + std::sin(azimuth) * bitangent);
        const float speed = style.minSpeed + (style.maxSpeed - style.minSpeed) * random.unit();
        const float fraction = static_cast<float>((static_cast<double>(i) + 0.5) / count);

        Particle particle;
        particle.position = from + (to - from) * fraction;
        particle.velocity = speed * heading;
        particle.color = style.startColor;
        particle.size = style.startSize;
        particle.rotation = glm::two_pi<float>() * random.unit();
        particle.lifeTicks = style.minLifeTicks + static_cast<int>(random.next() % lifeRange);
        particle.styleId = styleId;
        particles_.push_back(particle);
    }
}

}
