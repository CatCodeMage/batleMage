#pragma once
#include "Transform.h"
#include "MeshLibrary.h"      

struct Entidad {
    MeshId    mesh = INVALID_MESH;
    Transform transform;
    glm::vec4 color = glm::vec4(1.0f);
    int       kind = 0;                // 0 = liso, 1 = tablero
};