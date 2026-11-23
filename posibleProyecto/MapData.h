#pragma once
#include "global.h"
#include "Entidad.h"
#include "Renderer.h"

struct MapData {
    std::vector<Entidad> entities;
    std::vector<SceneLight> m_lights;

    bool load(const std::filesystem::path& file, const ModelLibrary& lib);
};

