#include "drawObjects.h"

// drawObjects.cpp
void submitEntidad(Renderer& renderer, const Entidad& e) {
    renderer.submit(e.mesh, e.transform.toMatrix(), e.color, e.kind);
}

void submitEntidades(Renderer& renderer, const std::vector<Entidad>& entities) {
    for (auto e : entities) {
        renderer.submit(e.mesh, e.transform.toMatrix(), e.color, e.kind);
    }
}