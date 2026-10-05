#pragma once

#include "map.h"
#include "assets.h"

#include <vector>

enum class skybox{
    CIELO_AZUL
}

class World
{   
public:
    World(/* args */);
    ~World();

private:
    Map* m_map;
    std::vector<Assets*> m_assets;
    fondo m_skybox;
};
