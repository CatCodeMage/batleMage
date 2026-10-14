#pragma once
#include "Mesh.h"
#include "MeshLibrary.h"
#include "ShadowMap.h"

struct Light {
    glm::vec3 pos;   float radius;      // hasta donde llega la luz
    glm::vec3 color; float intensity;
};

static_assert(sizeof(Light) == 32, "Light debe ser 2 vec4 contiguos");

constexpr int MAX_LIGHTS = 16; 

class Renderer {
public:
    Renderer();
    void init(GLuint program);
    void destroy(); // Antes de destruir la ventana; el programa de color pertenece al llamante.
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    void setShadowsEnabled(bool enabled) { w_shadowsEnabled = enabled; }
    bool shadowsReady() const { return w_shadowMap.ready() && w_shadowProgram != 0; }
    // Indice segun el orden de submitLight. -1 desactiva la pasada de sombras.
    void setShadowLightIndex(int index) { w_shadowLightIndex = index; }
    void submit(MeshId id, const glm::mat4& model, const glm::vec4& color, int kind = 0);
    void flush(const MeshLibrary& lib, const glm::mat4& viewProj);
    void submitLight(const Light& light);
private:
    void drawBatches(const MeshLibrary& lib) const;
    void renderShadowMap(const MeshLibrary& lib, const Light& light);
    ShadowMap w_shadowMap;
    GLuint w_program = 0, w_shadowProgram = 0;
    bool w_shadowsEnabled = true;
    int w_shadowLightIndex = 0;
    std::vector<Light> w_lights;
    std::vector<std::vector<InstanceData>> w_batches;     // un lote por MeshId
    GLint w_locViewProj, w_locNumLights, w_locLights;
    GLint w_locShadowLight = -1, w_locShadowFar = -1, w_locShadowTexel = -1;
    GLint w_locLightViewProj = -1, w_locLightPos = -1, w_locFarPlane = -1;
};
