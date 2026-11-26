#pragma once
#include "Transform.h"
#include "ModelLibrary.h"

using AnimationId = uint32_t;
constexpr AnimationId NO_ANIMATION = UINT32_MAX;

struct ModelAnimation {
    std::vector<ModelId> frames;
    double fps = 1.0;
    double spinRadiansPerSecond = 0.0;
    bool loop = true;
};

struct Entidad {
    ModelId   model = INVALID_MODEL;
    Transform transform;
    glm::vec4 color = glm::vec4(1.0f);   // tinte: se multiplica por el color de cada parte
    int       kind = 0;                  // 0 = liso, 1 = tablero
    AnimationId animation = NO_ANIMATION;
    // A negative age selects the replay clock for legacy samples.
    double animationAge = -1.0;
    bool   isIceLance = false;
    bool   isFireBall = false;
    bool   isLightningBolt = false;
    bool   isEarthSpike = false;
    bool   isWindBlade = false;
    bool   isWaterBolt = false;
    bool   isDaggerBarrage = false;
    int    poolIndex = -1;
};

struct ImpactVFX {
    std::string type;
    glm::vec3   position = glm::vec3(0.0f);
    glm::vec3   normal   = glm::vec3(0.0f, 1.0f, 0.0f);
    int         startTick = 0;
    float       duration  = 1.9f;
};
