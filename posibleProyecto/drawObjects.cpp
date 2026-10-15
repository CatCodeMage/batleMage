#include "drawObjects.h"

// drawObjects.cpp
void submitEntidad(Renderer& renderer, const Entidad& e) {
    renderer.submit(e.mesh, e.transform.toMatrix(), e.color, e.kind);
}