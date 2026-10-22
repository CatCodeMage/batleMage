#pragma once
#include "Mesh.h"
#include "MeshLibrary.h"

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

class Renderer {
public:
    Renderer();
    void init(GLuint program);
    void submit(MeshId id, GLuint albedoTexture, const glm::mat4& model, const glm::vec4& color, int kind = 0);
    void flush(const MeshLibrary& lib, const glm::mat4& viewProj);
    void drawShadowGeometry(const MeshLibrary& lib) const;
    void submitLight(const Light& light);
private:
    void getActualViewProj(const glm::mat4& viewProj);
    void flushLigth();
    std::vector<Light> w_lights;
    std::vector<std::vector<InstanceData>> w_batches;      
    std::vector<GLuint> w_batchTextures;   
    GLint w_locViewProj, w_locNumLights, w_locLights, w_locAlbedo;
};