#pragma once
#include "global.h"
#include "MeshLibrary.h"

using ModelId = uint32_t;
constexpr ModelId INVALID_MODEL = UINT32_MAX;

struct ModelPart {
    MeshId    mesh          = INVALID_MESH;
    glm::vec4 color         = glm::vec4(1.0f);
    GLuint    albedoTexture = 0;
};

struct Model {
    std::vector<ModelPart> parts;
};

class ModelLibrary {
private:
    std::vector<Model>                       w_models;   // indice = ModelId
    std::unordered_map<std::string, ModelId> w_byName;
public:
    ModelId      add(const std::string& name, const Model& model);
    ModelId      find(const std::string& name) const;   // assert si no existe
    const Model& get(ModelId id) const;
    bool         has(const std::string& name) const { return w_byName.count(name) != 0; }
};