#pragma once
#include "global.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"
#include "TextureLibrary.h"
#include "Material.h"
#include "MaterialLibrary.h"

struct ObjMaterial {
    std::string           name;                   
    glm::vec4             color = glm::vec4(1.0f); 
    std::filesystem::path albedoTexture;            
    glm::vec3             emission = glm::vec3(0.0f);
};

struct ObjPiece {
    std::string           shapeName;
    int                   materialId;
    ObjMaterial           material;
    std::vector<float>    verts;
    std::vector<unsigned> idx;

    ObjPiece();
};

std::vector<ObjPiece> loadObj(const std::filesystem::path& file);

int loadAssetManifest(const std::filesystem::path& manifest, MeshLibrary& meshes, ModelLibrary& models,
    MaterialLibrary& matLib, TextureLibrary& textures, size_t maxBones);
