#pragma once
#include "Transform.h"
#include "ModelLibrary.h"

using AnimationId = uint32_t;
constexpr AnimationId NO_ANIMATION = UINT32_MAX;

struct ModelAnimation {
    std::vector<ModelId> frames;
    double fps = 1.0;
    bool loop = true;
};

struct Entidad {
    ModelId   model = INVALID_MODEL;
    Transform transform;
    glm::vec4 color = glm::vec4(1.0f);   // tinte: se multiplica por el color de cada parte
    int       kind = 0;                  // 0 = liso, 1 = tablero
    AnimationId animation = NO_ANIMATION;
};
