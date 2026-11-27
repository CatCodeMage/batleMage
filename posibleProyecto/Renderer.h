#pragma once
#include <cmath>
#include "Mesh.h"
#include "MeshLibrary.h"
#include "MaterialLibrary.h"

struct Light {
    glm::vec3 pos;   float radius;      // hasta donde llega la luz
    glm::vec3 color; float intensity;
};

static_assert(sizeof(Light) == 32, "Light debe ser 2 vec4 contiguos");

// Configuracion adicional de cada luz. Para poder añadir información sin modificar el flush
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
    void flush(const MeshLibrary& lib, const glm::mat4& viewProj, const MaterialLibrary& matLib);
    void drawShadowGeometry(const MeshLibrary& lib) const;
    void submitLights(std::vector<SceneLight>& lights, const float time);
    void submitLight(const Light& light);
    // El programa ya debe estar activo.
    void applyLights(GLuint program) const;
private:
    void getActualViewProj(const glm::mat4& viewProj);
    std::vector<Light> w_lights;
    std::vector<std::vector<RenderBatch>> w_batches;
    GLuint w_program = 0;
    GLint w_locViewProj;
    GLint m_locAlbedoColor;
    GLint m_locEmissive;
    GLint m_locRoughness;
    GLint m_locMetallic;
    GLint m_locOpacity;
};
