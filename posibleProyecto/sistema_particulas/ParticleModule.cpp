#include "ParticleModule.h"

#include <utility>

namespace bm::particles {

ParticleModule::ParticleModule(std::size_t capacity) : system_(capacity), playback_(system_) {}

void ParticleModule::initialize(ParticleStyleCatalog styles, ParticleMaterialTable materials) {
    system_.setStyles(std::move(styles));
    playback_.reset();
    renderer_.initialize(std::move(materials));
}

void ParticleModule::setFrames(std::vector<ParticleFrame> frames, int ticksPerSecond) {
    playback_.setFrames(std::move(frames), ticksPerSecond);
}

void ParticleModule::seek(int tick) {
    playback_.seek(tick);
}

void ParticleModule::render(const glm::mat4& view, const glm::mat4& projection) {
    renderer_.render(system_.particles(), system_.styles(), view, projection);
}

void ParticleModule::shutdown() {
    renderer_.shutdown();
}

}
