#include "ParticleTestScene.h"

#include "ParticleEffect.h"

#include <cmath>
#include <stdexcept>

namespace bm::particles {

std::vector<ParticleFrame> ParticleTestScene::makeFrames(Scenario scenario, int frameCount, int ticksPerSecond) {
    if (frameCount < 0 || ticksPerSecond <= 0) throw std::invalid_argument("Duracion o frecuencia de prueba invalida");
    std::vector<ParticleFrame> frames(frameCount);

    switch (scenario) {
    case Scenario::Smoke:
        for (auto& frame : frames) {
            frame.emitters.push_back({.emitterId = 1, .styleId = presets::Smoke, .position = {-2.0f, 0.3f, 0.0f}, .seed = 17});
        }
        break;

    case Scenario::MagicTrail:
        for (int tick = 0; tick < frameCount; ++tick) {
            const float seconds = static_cast<float>(tick) / ticksPerSecond;
            if (seconds >= 3.0f) break;
            const float angle = 2.0f * seconds;
            frames[tick].emitters.push_back({
                .emitterId = 2, .styleId = presets::MagicTrail,
                .position = {3.0f * std::cos(angle), 1.0f + 0.4f * seconds, 3.0f * std::sin(angle)},
                .direction = {6.0f * std::sin(angle), -0.4f, -6.0f * std::cos(angle)}, .seed = 31
            });
        }
        break;

    case Scenario::Explosion:
        if (frameCount > ExplosionTick) {
            frames[ExplosionTick].bursts.push_back({
                .eventId = 1, .styleId = presets::Explosion, .position = {0.0f, 1.0f, 0.0f}, .count = 110, .seed = 59
            });
        }
        break;
    }
    return frames;
}

}
