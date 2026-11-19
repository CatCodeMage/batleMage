#include "drawObjects.h"

void submitEntidad(Renderer& renderer, const Model& model, const Entidad& e) {
    const glm::mat4 modelMatrix = e.transform.toMatrix();   // una vez por entidad, no por parte
    for (const ModelPart& part : model.parts)
        renderer.submit(part.mesh, part.albedoTexture, modelMatrix, part.color * e.color, e.kind);
}

void submitEntidad(bm::anim::SkinnedRenderer& renderer, const SkinnedModel& model, const Entidad& e,
    const bm::anim::Animator& animator) {
    const glm::mat4 characterModel = e.transform.toMatrix();
    for (size_t i = 0; i < model.parts.size(); ++i) {
        const auto& part = model.parts[i];
        renderer.submit(part.mesh, characterModel * animator.meshTransform(i),
            animator.boneMatrices(i), part.albedoTexture, e.color, e.kind);
    }
}
