#pragma once
#include "global.h"
#include "MeshLibrary.h"
#include "skeleton_animation/Animator.h"
#include "skeleton_animation/SkinnedMesh.h"

using ModelId = uint32_t;
constexpr ModelId INVALID_MODEL = UINT32_MAX;

struct ModelPart {
    MeshId    mesh          = INVALID_MESH;
    glm::vec4 color         = glm::vec4(1.0f);
    GLuint    albedoTexture = 0;
};

struct SkinnedPart {
    bm::anim::SkinnedMesh mesh;
    GLuint albedoTexture;
};

struct SkinnedModel {
    explicit SkinnedModel(std::shared_ptr<const bm::anim::ImportedModel> imported);
    std::shared_ptr<const bm::anim::AnimationClip> findClip(const std::string& name) const;

    std::shared_ptr<const bm::anim::Skeleton> skeleton;
    bm::anim::Animator bindPose;
    std::vector<SkinnedPart> parts;
    std::unordered_map<std::string, std::shared_ptr<const bm::anim::AnimationClip>> clips;
    std::unordered_map<std::string, std::string> animations;
};

struct Model {
    std::vector<ModelPart> parts;
    std::unique_ptr<SkinnedModel> skinned;
};

class ModelLibrary {
private:
    std::vector<Model>                       w_models;   // indice = ModelId
    std::unordered_map<std::string, ModelId> w_byName;
public:
    ModelId      add(const std::string& name, Model model);
    ModelId      find(const std::string& name) const;   // assert si no existe
    const Model& get(ModelId id) const;
    bool         has(const std::string& name) const { return w_byName.count(name) != 0; }
    void         destroy();
};
