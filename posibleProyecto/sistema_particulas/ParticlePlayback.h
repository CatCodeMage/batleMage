#pragma once

#include "ParticleFrame.h"

#include <vector>

namespace bm::particles {

class ParticleSystem;

class ParticlePlayback {
public:
    // El sistema debe vivir mientras se use la reproduccion.
    explicit ParticlePlayback(ParticleSystem& system);
    ParticlePlayback(const ParticlePlayback&) = delete;
    ParticlePlayback& operator=(const ParticlePlayback&) = delete;

    // Cada posicion del vector representa un tick, incluidos los ticks vacios.
    void setFrames(std::vector<ParticleFrame> frames, int ticksPerSecond);
    void seek(int targetTick); // Limita el tick al intervalo disponible.
    void reset(); // Vuelve al estado previo al tick 0, conservando los fotogramas.
    int currentTick() const { return currentTick_; }

private:
    ParticleSystem& system_;
    std::vector<ParticleFrame> frames_;
    int ticksPerSecond_ = 1;
    int currentTick_ = -1;
};

}
