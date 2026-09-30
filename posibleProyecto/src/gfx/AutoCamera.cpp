#include "gfx/AutoCamera.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

namespace gfx {

void AutoCamera::update(const sim::Battle& battle, float dt) {
    const sim::Fighter& a = battle.fighter(0);
    const sim::Fighter& b = battle.fighter(1);

    const glm::vec3 mid = (a.pos + b.pos) * 0.5f;
    glm::vec3 axis = b.pos - a.pos;
    axis.y = 0.0f;
    const float sep = glm::length(axis);
    axis = sep > 1e-4f ? axis / sep : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 side(axis.z, 0.0f, -axis.x);   // perpendicular horizontal al eje del combate

    glm::vec3 focus = mid + glm::vec3(0.0f, 1.2f, 0.0f);
    float dist = std::clamp(sep * 0.75f + 4.0f, 7.5f, 13.0f);
    const float swing = 0.55f * std::sin(0.23f * battle.time());   // vaiven respecto a la vista lateral

    // Cierre: primer plano del ganador
    const sim::Outcome o = battle.outcome();
    if (o == sim::Outcome::Fighter0Wins || o == sim::Outcome::Fighter1Wins) {
        const sim::Fighter& w = battle.fighter(o == sim::Outcome::Fighter0Wins ? 0 : 1);
        focus = w.pos + glm::vec3(0.0f, 1.3f, 0.0f);
        dist = 6.0f;
    }

    const glm::vec3 dir = side * std::cos(swing) + axis * std::sin(swing);
    const glm::vec3 desiredPos = focus + dir * dist + glm::vec3(0.0f, 1.8f + 0.15f * dist, 0.0f);

    if (!initialized_) {
        pos_ = desiredPos;
        target_ = focus;
        initialized_ = true;
    } else {
        const float k = 1.0f - std::exp(-3.0f * dt);   // suavizado exponencial
        pos_ += (desiredPos - pos_) * k;
        target_ += (focus - target_) * k;
    }
}

glm::mat4 AutoCamera::view() const {
    return glm::lookAt(pos_, target_, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 AutoCamera::projection(float aspect) const {
    return glm::perspective(glm::radians(45.0f), aspect, 0.1f, 120.0f);
}

}  // namespace gfx
