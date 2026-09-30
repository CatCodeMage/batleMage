#pragma once
#include <cstdint>

namespace sim {

// Generador pseudoaleatorio determinista (xorshift64*).
// Misma semilla => misma secuencia en cualquier ordenador. Es lo que permite
// reproducir y volver a grabar exactamente la misma batalla.
class Rng {
public:
    explicit Rng(std::uint64_t seed = 1) : state_(seed ? seed : 0x9E3779B97F4A7C15ull) {}

    std::uint64_t nextU64() {
        state_ ^= state_ >> 12;
        state_ ^= state_ << 25;
        state_ ^= state_ >> 27;
        return state_ * 2685821657736338717ull;
    }

    float uniform() { return static_cast<float>(nextU64() >> 40) * (1.0f / 16777216.0f); }  // [0,1)
    float range(float a, float b) { return a + (b - a) * uniform(); }
    bool chance(float p) { return uniform() < p; }

private:
    std::uint64_t state_;
};

}  // namespace sim
