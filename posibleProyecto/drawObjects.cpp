#include "drawObjects.h"

void submitEntidad(Renderer& renderer, const ModelLibrary& models, const Entidad& e) {
    const glm::mat4 modelMatrix = e.transform.toMatrix();   // una vez por entidad, no por parte
    for (const ModelPart& part : models.get(e.model).parts)
        renderer.submit(part.mesh, part.albedoTexture, modelMatrix, part.color * e.color, e.kind);
}

void submitEntidades(Renderer& renderer, const ModelLibrary& models, const std::vector<Entidad>& entities) {
    for (const Entidad& e : entities) submitEntidad(renderer, models, e);
}