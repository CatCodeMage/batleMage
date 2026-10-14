#pragma once
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>

// Sin OpenGL: permite probar las seis camaras de la luz sin crear una ventana.
namespace ShadowMath {
    constexpr float NEAR_PLANE = 0.1f;

    inline bool validLight(const glm::vec3& position, float radius) {
        return std::isfinite(position.x) && std::isfinite(position.y) &&
            std::isfinite(position.z) && std::isfinite(radius) && radius > NEAR_PLANE;
    }

    // Orden exigido por OpenGL: +X, -X, +Y, -Y, +Z, -Z.
    // Cada cara ve 90 grados. Los vectores up evitan singularidades en +/-Y
    // y hacen coincidir la orientacion de las camaras con la del cubemap.
    inline std::array<glm::mat4, 6> faceMatrices(const glm::vec3& position, float radius) {
        const glm::vec3 directions[] = {
            {1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}
        };
        const glm::vec3 up[] = {
            {0, -1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}, {0, -1, 0}, {0, -1, 0}
        };
        const glm::mat4 projection = glm::perspective(glm::radians(90.0f), 1.0f,
            NEAR_PLANE, radius);
        std::array<glm::mat4, 6> matrices;
        for (size_t face = 0; face < matrices.size(); ++face)
            matrices[face] = projection * glm::lookAt(position, position + directions[face], up[face]);
        return matrices;
    }
}
