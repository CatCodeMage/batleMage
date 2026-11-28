#include "ModelLibrary.h"

SkinnedModel::SkinnedModel(std::shared_ptr<const bm::anim::ImportedModel> imported)
    : skeleton(std::make_shared<bm::anim::Skeleton>(imported)), bindPose(skeleton) {
    for (size_t i = 0; i < imported->clips.size(); ++i) {
        auto clip = std::make_shared<bm::anim::AnimationClip>(imported, i);
        if (!clips.emplace(clip->name(), clip).second)
            throw std::runtime_error("nombre de clip repetido: " + clip->name());
    }
}

std::shared_ptr<const bm::anim::AnimationClip> SkinnedModel::findClip(const std::string& action) const {
    const auto alias = animations.find(action);
    const auto clip = clips.find(alias == animations.end() ? action : alias->second);
    return clip == clips.end() ? nullptr : clip->second;
}

ModelId ModelLibrary::add(const std::string& name, Model model) {
    assert(!has(name));
    const ModelId id = (ModelId)w_models.size();
    w_models.push_back(std::move(model));
    w_byName[name] = id;
    return id;
}

ModelId ModelLibrary::find(const std::string& name) const {
    std::unordered_map<std::string, ModelId>::const_iterator iterator = w_byName.find(name);
    assert(iterator != w_byName.end());
    return iterator->second;
}

const Model& ModelLibrary::get(ModelId id) const {
    assert(id < w_models.size());
    return w_models[id];
}

void ModelLibrary::destroy() {
    w_models.clear();
    w_byName.clear();
}
