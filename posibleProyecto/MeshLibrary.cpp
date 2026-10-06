#include "MeshLibrary.h"

MeshId MeshLibrary::add(const std::string& name, const std::vector<float>& verts,
    const std::vector<unsigned>& idx) {
    MeshId id = (MeshId)w_meshes.size();
    w_meshes.emplace_back();
    w_meshes.back().create(verts, idx);
    w_byName[name] = id;
    return id;
}

MeshId MeshLibrary::find(const std::string& name) const {
    std::unordered_map<std::string, MeshId>::const_iterator iterator = w_byName.find(name);
    assert(iterator != w_byName.end());
    return iterator->second;
}

const Mesh& MeshLibrary::get(MeshId id) const {
    assert(id < w_meshes.size()); 
    return w_meshes[id]; 
}

void MeshLibrary::destroy() {
    for (int i = 0; i < w_meshes.size(); i++)
        w_meshes[i].destroy();
}