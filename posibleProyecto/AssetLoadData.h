#pragma once
#include "global.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"
#include "TextureLibrary.h"

struct ObjMaterial {
    std::string           name;                   
    glm::vec4             color = glm::vec4(1.0f); 
    std::filesystem::path albedoTexture;            
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
    TextureLibrary& textures);