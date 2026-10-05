#pragma once

#include <variant>
#include <glm/glm.hpp>

#include "Collision.h"


enum class tipoAsset
{
    ARBOL
};

enum class estadoArbol{
    ENTERO,
    DESTRUIDO
};

using estadoAsset = std::variant<
    estadoArbol
>;



class Assets
{
public:
    Assets(/* args */);
    ~Assets();

private:
    BoxCollider m_hitbox;
    tipoAsset m_tipo;
    estadoAsset m_estado;

    glm::vec3 m_coordenadas;
};
