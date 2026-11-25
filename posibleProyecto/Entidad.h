#pragma once
#include "Transform.h"
#include "ModelLibrary.h"
#include "GeneradorIds.h"   // solo el tipo EntityId (sin dependencias)

struct Entidad {
    ModelId   model = INVALID_MODEL;
    Transform transform;
    glm::vec4 color = glm::vec4(1.0f);   // tinte: se multiplica por el color de cada parte
    int       kind = 0;                  // 0 = liso, 1 = tablero
    EntityId  id = INVALID_ENTITY;       // id de instancia (magos y hechizos); 0 = sin id. Lo usan las camaras
    int       characterId = 0;           // 0 si el replay no identifica a un combatiente
    std::string animation;
    int       animationStartFrame = 0;
};
