#include "ModelLibrary.h"

ModelId ModelLibrary::add(const std::string& name, const Model& model) {
    assert(!has(name));
    const ModelId id = (ModelId)w_models.size();
    w_models.push_back(model);
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