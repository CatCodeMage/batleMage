#pragma once

#include "ParticleEffect.h"

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace bm::particles {

struct Particle {
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    glm::vec4 color{1.0f};
    float size = 1.0f;
    float rotation = 0.0f; // Radianes.
    int ageTicks = 0;
    int lifeTicks = 1;
    ParticleStyleId styleId = 0;
};

}
