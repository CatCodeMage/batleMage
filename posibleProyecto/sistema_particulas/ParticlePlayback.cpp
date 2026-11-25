#include "ParticlePlayback.h"
#include "ParticleSystem.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bm::particles {

ParticlePlayback::ParticlePlayback(ParticleSystem& system) : system_(system) {}

void ParticlePlayback::setFrames(std::vector<ParticleFrame> frames, int ticksPerSecond) {
    if (ticksPerSecond <= 0) throw std::invalid_argument("La frecuencia del replay debe ser positiva");
    if (frames.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::length_error("Demasiados fotogramas de particulas");
    }
    frames_ = std::move(frames);
    ticksPerSecond_ = ticksPerSecond;
    reset();
}

void ParticlePlayback::seek(int targetTick) {
    if (frames_.empty()) return;

    const int tick = std::clamp(targetTick, 0, static_cast<int>(frames_.size()) - 1);
    if (tick < currentTick_) reset();
    try {
        while (currentTick_ < tick) {
            const int nextTick = currentTick_ + 1;
            system_.step(frames_[nextTick], nextTick, ticksPerSecond_);
            currentTick_ = nextTick;
        }
    } catch (...) {
        // Un tick parcial no se puede reintentar sin reconstruir el estado.
        reset();
        throw;
    }
}

void ParticlePlayback::reset() {
    system_.reset();
    currentTick_ = -1;
}

}
