#include "../ParticlePlayback.h"
#include "../ParticleSystem.h"
#include "../ParticleTestScene.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace bm::particles;

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<Particle> snapshot(const ParticleSystem& system) {
    const auto particles = system.particles();
    return {particles.begin(), particles.end()};
}

bool sameParticles(std::span<const Particle> first, std::span<const Particle> second) {
    if (first.size() != second.size()) return false;
    for (std::size_t i = 0; i < first.size(); ++i) {
        const auto& a = first[i];
        const auto& b = second[i];
        if (a.position != b.position || a.velocity != b.velocity || a.color != b.color || a.size != b.size ||
            a.rotation != b.rotation || a.ageTicks != b.ageTicks || a.lifeTicks != b.lifeTicks || a.styleId != b.styleId) return false;
    }
    return true;
}

void testBirthAndFixedStep() {
    const ParticleStyle style{
        .minLifeTicks = 3, .maxLifeTicks = 3, .minSpeed = 2.0f, .maxSpeed = 2.0f,
        .acceleration = {4.0f, 0.0f, 0.0f}, .startSize = 2.0f, .endSize = 0.0f,
        .startColor = {1.0f, 0.5f, 0.0f, 0.75f}, .endColor = {0.0f, 0.5f, 1.0f, 0.0f}
    };
    ParticleSystem system;
    system.setStyles({{1, style}});
    ParticlePlayback playback(system);
    std::vector<ParticleFrame> frames(4);
    frames[0].bursts.push_back({.eventId = 10, .styleId = 1, .position = {1.0f, 2.0f, 3.0f},
                                .direction = {5.0f, 0.0f, 0.0f}, .count = 1, .seed = 42});
    playback.setFrames(std::move(frames), 20);

    playback.seek(0);
    require(system.particles().size() == 1, "birth count");
    const auto birth = system.particles().front();
    require(birth.ageTicks == 0 && birth.lifeTicks == 3 && birth.styleId == 1, "initial lifetime and style");
    require(birth.position == glm::vec3(1, 2, 3) && birth.velocity == glm::vec3(2, 0, 0), "initial transform");
    require(birth.size == style.startSize && birth.color == style.startColor, "initial size and color");

    playback.seek(1);
    const auto updated = system.particles().front();
    require(updated.ageTicks == 1 && std::abs(updated.velocity.x - 2.2f) < 0.00001f, "fixed-step velocity");
    require(std::abs(updated.position.x - 1.11f) < 0.00001f && updated.position.y == 2.0f, "fixed-step position");
    require(updated.color == glm::vec4(0.5f, 0.5f, 0.5f, 0.375f) && updated.size == 1.0f, "interpolation");

    playback.seek(2);
    require(system.particles().front().color == style.endColor && system.particles().front().size == style.endSize, "final appearance");
    playback.seek(3);
    require(system.particles().empty(), "expiration");
}

void testContinuousRate() {
    for (int frequency : {30, 60}) {
        // Enteros para calcular los nacimientos esperados sin repetir la formula en coma flotante.
        for (int birthsInTwoSeconds : {20, 47, 80, 150, 290}) {
            ParticleSystem system;
            system.setStyles({{1, {.particlesPerSecond = birthsInTwoSeconds / 2.0f, .minLifeTicks = 200, .maxLifeTicks = 200}}});
            ParticlePlayback playback(system);
            const int lastTick = 2 * frequency + 1;
            std::vector<ParticleFrame> frames(lastTick + 1);
            for (auto& frame : frames) frame.emitters.push_back({.emitterId = 1, .styleId = 1, .startTick = 2});
            playback.setFrames(std::move(frames), frequency);
            for (int tick = 0; tick <= lastTick; ++tick) {
                playback.seek(tick);
                const auto expected = static_cast<std::size_t>(tick < 2 ? 0 : (tick - 1) * birthsInTwoSeconds / (2 * frequency));
                require(system.particles().size() == expected, "continuous rate per tick");
            }
            require(system.particles().size() == static_cast<std::size_t>(birthsInTwoSeconds),
                    "equal emission at both tick rates over two seconds");
        }
    }
}

void testPool() {
    ParticleSystem system(2);
    system.setStyles({{1, {.minLifeTicks = 2, .maxLifeTicks = 2}}});
    ParticlePlayback playback(system);
    std::vector<ParticleFrame> frames(5);
    frames[0].bursts.push_back({.eventId = 1, .styleId = 1, .count = 100});
    frames[1].bursts.push_back({.eventId = 2, .styleId = 1, .position = {9.0f, 0.0f, 0.0f}, .count = 1});
    frames[2].bursts.push_back({.eventId = 3, .styleId = 1, .position = {2.0f, 0.0f, 0.0f}, .count = 1});
    playback.setFrames(std::move(frames), 60);

    playback.seek(0);
    require(system.particles().size() == 2, "pool capacity");
    const Particle* storage = system.particles().data();
    playback.seek(1);
    require(system.particles().size() == 2 && system.particles().front().ageTicks == 1 &&
        system.particles().front().position.x == 0.0f, "saturation preserves existing particles");
    playback.seek(2);
    require(system.particles().size() == 1 && system.particles().data() == storage &&
        system.particles().front().position.x == 2.0f && system.particles().front().ageTicks == 0, "reuse on expiry");
    playback.seek(4);
    require(system.particles().empty(), "reused particle expires");
}

void testOneTickLifeAndBurstPriority() {
    ParticleSystem system(2);
    system.setStyles({{1, {.particlesPerSecond = 120.0f, .startSize = 0.0f, .endSize = 4.0f,
                          .startColor = {1.0f, 1.0f, 1.0f, 0.0f}}}});
    ParticlePlayback playback(system);
    std::vector<ParticleFrame> frames(2);
    frames[0].emitters.push_back({.emitterId = 1, .styleId = 1});
    frames[0].bursts.push_back({.eventId = 2, .styleId = 1, .position = {8.0f, 0.0f, 0.0f}, .count = 1});
    frames[0].bursts.push_back({.eventId = 1, .styleId = 1, .position = {4.0f, 0.0f, 0.0f}, .count = 8});
    playback.setFrames(frames, 60);
    playback.seek(0);
    const auto particles = system.particles();
    require(particles.size() == 2 && particles[0].position.x == 8.0f && particles[1].position.x == 4.0f,
            "bursts take free slots before continuous emitters, in input order");
    require(particles.front().lifeTicks == 1 && particles.front().ageTicks == 0 &&
            particles.front().size == 0.0f && particles.front().color.a == 0.0f, "one-tick particle uses birth appearance");
    playback.seek(1);
    require(system.particles().empty(), "one-tick particle expires on the next tick");

    ParticleSystem emptyPool(0);
    emptyPool.setStyles(system.styles());
    ParticlePlayback emptyPlayback(emptyPool);
    emptyPlayback.setFrames(std::move(frames), 60);
    emptyPlayback.seek(1);
    require(emptyPlayback.currentTick() == 1 && emptyPool.particles().empty(), "zero capacity accepts all replay ticks");
}

void testMovingEmitter() {
    ParticleSystem system;
    system.setStyles({{1, {.particlesPerSecond = 40.0f, .minLifeTicks = 20, .maxLifeTicks = 20}}});
    ParticlePlayback playback(system);
    std::vector<ParticleFrame> frames(4);
    frames[0].emitters.push_back({.emitterId = 1, .styleId = 1});
    frames[1].emitters.push_back({.emitterId = 1, .styleId = 1, .position = {8.0f, 0.0f, 0.0f}});
    frames[3].emitters.push_back({.emitterId = 1, .styleId = 1, .position = {30.0f, 0.0f, 0.0f}});
    playback.setFrames(std::move(frames), 20);
    playback.seek(1);
    const auto trail = system.particles();
    require(trail.size() == 4 && trail[0].position.x == 0.0f && trail[1].position.x == 0.0f, "old particles stay in world space");
    require(trail[2].position.x == 2.0f && trail[3].position.x == 6.0f, "births spread along emitter path");
    playback.seek(2);
    require(system.particles().size() == 4 && system.particles()[0].ageTicks == 2, "emitter removal stops births only");
    playback.seek(3);
    require(system.particles().size() == 6 && system.particles().back().position.x == 30.0f, "no bridge across missing ticks");
}

void testEmitterIdentity() {
    ParticleSystem system;
    const ParticleStyle style{.particlesPerSecond = 20.0f, .minLifeTicks = 20, .maxLifeTicks = 20,
                              .minSpeed = 1.0f, .maxSpeed = 1.0f};
    system.setStyles({{1, style}, {2, style}});
    ParticlePlayback playback(system);
    std::vector<ParticleFrame> frames(8);
    const ParticleEmitterState original{.emitterId = 1, .styleId = 1, .seed = 17};
    auto moved = original;
    moved.position.x = 8.0f;
    moved.direction = {1.0f, 0.0f, 0.0f};
    frames[0].emitters = {original, moved};
    frames[1].emitters = {moved};
    auto invalid = moved;
    invalid.seed = 18;
    frames[2].emitters = {invalid, moved};
    invalid.seed = 17;
    invalid.styleId = 2;
    frames[3].emitters = {invalid};
    invalid.styleId = 1;
    invalid.startTick = 4;
    frames[4].emitters = {invalid};
    moved.position.x = 16.0f;
    frames[5].emitters = {moved};
    frames[7].emitters = {{.emitterId = 1, .styleId = 2, .position = {30.0f, 0.0f, 0.0f}, .startTick = 7, .seed = 25}};
    playback.setFrames(std::move(frames), 20);
    playback.seek(0);
    require(system.particles().size() == 1 && system.particles().front().position.x == 0.0f, "first duplicate wins");
    playback.seek(1);
    require(system.particles().size() == 2 && system.particles().back().position.x == 4.0f &&
            system.particles().back().velocity.x == 1.0f, "position and direction may change");
    playback.seek(4);
    require(system.particles().size() == 2, "changing seed, style or start tick must not emit");
    playback.seek(5);
    require(system.particles().size() == 3 && system.particles().back().position.x == 12.0f, "invalid updates preserve last valid position");
    playback.seek(7);
    require(system.particles().size() == 4 && system.particles().back().styleId == 2 &&
            system.particles().back().position.x == 30.0f, "ID can start a different lifetime after disappearing");
}

void testReplayAndSeeds() {
    using Scenario = ParticleTestScene::Scenario;
    auto frames = ParticleTestScene::makeFrames(Scenario::Smoke);
    auto trail = ParticleTestScene::makeFrames(Scenario::MagicTrail);
    auto explosion = ParticleTestScene::makeFrames(Scenario::Explosion);
    for (std::size_t i = 0; i < frames.size(); ++i) {
        auto& frame = frames[i];
        for (const auto& emitter : trail[i].emitters) frame.emitters.push_back(emitter);
        frame.emitters.push_back({.emitterId = 3, .styleId = presets::Fire, .seed = 13});
        frame.bursts = std::move(explosion[i].bursts);
    }

    ParticleSystem system;
    system.setStyles(presets::makeCatalog());
    ParticlePlayback playback(system);
    playback.seek(100);
    require(playback.currentTick() == -1 && system.particles().empty(), "empty replay");
    playback.setFrames(frames, 60);
    playback.seek(0);
    const auto at0 = snapshot(system);
    for (int tick = 1; tick <= 100; ++tick) playback.seek(tick);
    const auto at100 = snapshot(system);
    playback.seek(100);
    require(playback.currentTick() == 100 && sameParticles(at100, system.particles()), "pause at 100");

    playback.seek(110);
    const auto at110 = snapshot(system);
    for (int tick = 111; tick <= 200; ++tick) playback.seek(tick);
    const auto at200 = snapshot(system);
    require(!at200.empty(), "reference particles at tick 200");
    playback.seek(300);
    playback.seek(200);
    require(sameParticles(at200, system.particles()), "rewind from 300 to 200");
    playback.seek(100);
    require(sameParticles(at100, system.particles()), "rewind from 200 to 100");
    for (int tick = 101; tick <= 110; ++tick) playback.seek(tick);
    require(sameParticles(at110, system.particles()), "ten-tick jump matches ten sequential steps");
    playback.seek(300);
    playback.seek(0);
    require(playback.currentTick() == 0 && sameParticles(at0, system.particles()), "rewind from 300 to 0");
    playback.seek(100);
    playback.reset();
    require(playback.currentTick() == -1 && system.particles().empty(), "reset");
    playback.seek(200);
    require(sameParticles(at200, system.particles()), "jump from start to 200");

    playback.seek(120);
    const auto at120 = snapshot(system);
    playback.setFrames(frames, 60);
    playback.seek(120);
    require(sameParticles(at120, system.particles()), "same input and seeds");
    ++frames[120].bursts.front().seed;
    playback.setFrames(std::move(frames), 60);
    playback.seek(120);
    require(at120.size() == system.particles().size() && !sameParticles(at120, system.particles()), "different seed changes the burst");
}

void testExplosionScene() {
    ParticleSystem system;
    system.setStyles(presets::makeCatalog());
    ParticlePlayback playback(system);
    playback.setFrames(ParticleTestScene::makeFrames(ParticleTestScene::Scenario::Explosion), 60);
    playback.seek(119);
    require(system.particles().empty(), "no burst before tick 120");
    playback.seek(120);
    const auto burst = snapshot(system);
    require(burst.size() == 110, "burst count at 120");
    playback.seek(120);
    require(sameParticles(burst, system.particles()), "burst not repeated on pause");
    playback.seek(121);
    require(system.particles().size() == 110 && system.particles().front().ageTicks == 1, "burst not repeated on next tick");
    playback.seek(359);
    require(system.particles().empty(), "burst particles expire");
}

}

int main() {
    try {
        testBirthAndFixedStep();
        testContinuousRate();
        testPool();
        testMovingEmitter();
        testEmitterIdentity();
        testOneTickLifeAndBurstPriority();
        testReplayAndSeeds();
        testExplosionScene();
        std::cout << "8 CPU particle tests passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
