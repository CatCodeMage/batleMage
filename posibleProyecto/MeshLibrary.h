#pragma once
#include "global.h"
#include "Mesh.h"

using MeshId = uint32_t;
constexpr MeshId INVALID_MESH = UINT32_MAX;

class MeshLibrary {
private:
    std::vector<Mesh> w_meshes;                        // índice = MeshId
    std::unordered_map<std::string, MeshId> w_byName;  // "nombreMesh" -> indice
public:
    MeshId add(const std::string& name, const std::vector<float>& verts,
        const std::vector<unsigned>& idx);
    MeshId find(const std::string& name) const;      // INVALID_MESH si no existe, corta ejecucion
    const Mesh& get(MeshId id) const; // Si se excede de rango corta ejecucion
    size_t count() const { return w_meshes.size(); }
    void destroy();                                   
};
