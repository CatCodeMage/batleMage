#pragma once

#include <glm/glm.hpp>

struct FuenteDeLuz
{
    glm::vec3 desplazamiento{0.0f};
    glm::vec3 color{1.0f};
    float intensidad{1.0f};
    float rango{10.0f};
};

struct LlumAmbient
{
    glm::vec3 color{1.0f};
    float intensitat{1.0f};
};