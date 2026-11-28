#pragma once

#include "ParticleFrame.h"

#include <vector>

namespace bm::particles {

class ParticleTestScene {
public:
    enum class Scenario { Smoke, MagicTrail, Explosion };
    static constexpr int ExplosionTick = 120;

    // El resultado se puede pasar directamente a ParticleModule::setFrames.
    static std::vector<ParticleFrame> makeFrames(Scenario scenario, int frameCount = 360, int ticksPerSecond = 60);
};

}
