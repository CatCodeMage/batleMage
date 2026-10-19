#pragma once
#include "global.h"
#include "MeshLibrary.h"


void loadObj(const std::filesystem::path& file, std::vector<float>& verts, std::vector<unsigned>& idx);

int loadAssetManifest(const std::filesystem::path& manifest, MeshLibrary& lib);
