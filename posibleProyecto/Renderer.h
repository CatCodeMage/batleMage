#pragma once

#include <cmath>
#include "Mesh.h"
#include "MeshLibrary.h"
#include "MaterialLibrary.h"

struct Light {
    glm::vec3 pos;
    float radius;

    glm::vec3 color;
    float intensity;
};

static_assert(sizeof(Light) == 32,
    "Light debe ser 2 vec4 contiguos");

struct SceneLight {
    Light light;
    bool castsShadow = false;
};

constexpr int MAX_LIGHTS = 16;

struct RenderBatch {
    MaterialId material = INVALID_MATERIAL_ID;
    std::vector<InstanceData> instances;
};

class Renderer {
public:
    Renderer();

    void init(GLuint program);
    void submit(MeshId id, MaterialId matId, const glm::mat4& model, const glm::vec4& color, int kind = 0);
    // se puede llamar una vez por vista: no vacia nada; clearFrame cuando ya se han pintado todas
    void drawLighting(const MeshLibrary& lib, const MaterialLibrary& matLib, const glm::mat4& viewProj,
        const glm::vec3& cameraPosition) const;
    void clearFrame();
    void drawShadowGeometry(const MeshLibrary& lib) const;

    void submitLights(std::vector<SceneLight>& lights, const float time);
    void submitLight(const Light& light);

    void applyLights(GLuint program) const;
private:
    std::vector<Light> w_lights;
    std::vector<std::vector<RenderBatch>> w_batches;

    GLuint w_program = 0;

    GLint w_locViewProj = -1;
    GLint w_locCamPos = -1;

    GLint m_locAlbedoColor = -1;
    GLint m_locEmissive = -1;
    GLint m_locRoughness = -1;
    GLint m_locMetallic = -1;
    GLint m_locOpacity = -1;
};