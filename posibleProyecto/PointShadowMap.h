
#pragma once

#include "global.h"
#include <functional>

// Recurso OpenGL de sombras de una unica luz puntual.
// Un cubemap de profundidad: 6 caras de 512 x 512.
class PointShadowMap {
public:
    static constexpr int Resolution = 512;

    using DrawFace = std::function<void(
        const glm::mat4& lightViewProjection
    )>;

    PointShadowMap() = default;
    ~PointShadowMap();

    PointShadowMap(const PointShadowMap&) = delete;
    PointShadowMap& operator=(const PointShadowMap&) = delete;

    // Crear los recursos OpenGL.
    bool initialize();

    // Dibuja las seis caras del cubemap.
    void render(
        const glm::vec3& lightPosition,
        float lightRadius,
        const DrawFace& drawFace
    );

    // Liberar recursos antes de destruir el contexto.
    void destroy();

    GLuint depthCubemap() const {
        return m_depthCubemap;
    }

    bool isInitialized() const {
        return m_depthCubemap != 0 &&
               m_framebuffer != 0;
    }

private:
    GLuint m_framebuffer = 0;   //frame buffer off-screen en memoria
    GLuint m_depthCubemap = 0;  //textura formada por 6 imagenes que almacena la profundidad
};
