#pragma once
#include "global.h"

struct Transform {
    glm::vec3 pos = glm::vec3(0.0f);
    glm::vec3 rot = glm::vec3(0.0f);   // radianes: x = cabeceo, y = guiñada, z = alabeo
    glm::vec4 color = glm::vec4(0.0f);   // radianes: x = cabeceo, y = guiñada, z = alabeo
    glm::vec3 scale = glm::vec3(1.0f);

    glm::mat4 toMatrix() const;
};