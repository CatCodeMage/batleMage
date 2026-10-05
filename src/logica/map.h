#pragma once

#include <glm/glm.hpp>

enum class tipoMapa
{
    
};

class Map
{
public:
    Map(/* args */);
    ~Map();

private:
    tipoMapa m_tipo;
    glm::vec3 m_dimensiones;
};