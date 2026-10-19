#pragma once
#include "global.h"
#include "Entidad.h"

struct MapData {
    std::vector<Entidad> entities;

    bool load(const std::filesystem::path& file, const MeshLibrary& lib);
};

