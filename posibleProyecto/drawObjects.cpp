#include "drawObjects.h"
#include <algorithm>
#include <cmath>

static ModelId modelAt(const Entidad& entity, std::span<const ModelAnimation> animations, double timeSeconds) {
    if (entity.animation == NO_ANIMATION) return entity.model;

    const ModelAnimation& animation = animations[entity.animation];
    const double visualFrame = std::floor(timeSeconds * animation.fps);
    const double index = animation.loop
        ? std::fmod(visualFrame, static_cast<double>(animation.frames.size()))
        : (std::min)(visualFrame, static_cast<double>(animation.frames.size() - 1));
    return animation.frames[static_cast<size_t>(index)];
}

void submitEntidad(Renderer& renderer, const ModelLibrary& models, const Entidad& e,
    std::span<const ModelAnimation> animations, double timeSeconds) {
    const glm::mat4 modelMatrix = e.transform.toMatrix();   // una vez por entidad, no por parte
    for (const ModelPart& part : models.get(modelAt(e, animations, timeSeconds)).parts)
        renderer.submit(part.mesh, part.albedoTexture, modelMatrix, part.color * e.color, e.kind, part.emission);
}

void submitEntidades(Renderer& renderer, const ModelLibrary& models, const std::vector<Entidad>& entities,
    std::span<const ModelAnimation> animations, double timeSeconds) {
    for (const Entidad& e : entities) submitEntidad(renderer, models, e, animations, timeSeconds);
}
