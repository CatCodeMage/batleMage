#pragma once
#include "Transform.h"
#include "ModelLibrary.h"

struct Entidad {
    ModelId   model = INVALID_MODEL;
    Transform transform;
    glm::vec4 color = glm::vec4(1.0f);   // tinte: se multiplica por el color de cada parte
    int       kind = 0;                  // 0 = liso, 1 = tablero
};