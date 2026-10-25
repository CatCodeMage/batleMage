
#pragma once

#include "PointShadowMap.h"
#include "Renderer.h"

#include <array>
#include <functional>
#include <vector>

class ShadowSystem {
public:
    // Maximo de luces que pueden proyectar sombras simultaneamente.
    static constexpr int MaxShadowLights = 4;

    // Usaremos las unidades de textura 4, 5, 6 y 7.
    static constexpr int FirstTextureUnit = 4;

    // Funcion que dibuja la geometria de la escena.
    using DrawGeometry = std::function<void()>;

    ShadowSystem();
    ~ShadowSystem();

    // No permitimos copiar recursos OpenGL.
    ShadowSystem(const ShadowSystem&) = delete;
    ShadowSystem& operator=(const ShadowSystem&) = delete;

    // Activar o desactivar todas las sombras.
    void setEnabled(bool enabled);

    bool isEnabled() const {
        return m_enabled;
    }

    // Generar los cubemaps de las luces con castsShadow = true.
    // Llamar antes de Renderer::flush().
    void render(
        const std::vector<SceneLight>& lights,
        const DrawGeometry& drawGeometry
    );

    // Enviar los cubemaps y sus uniforms al shader principal.
    void bindForLighting(GLuint lightingProgram) const;

    // Liberar recursos antes de destruir el contexto OpenGL.
    void destroy();

    // Numero de luces que tienen sombras activas en este frame.
    int activeShadowCount() const {
        return m_activeCount;
    }

    void procesInit();

private:
    // Crear el programa de profundidad.
    // Llamar despues de inicializar OpenGL.
    bool initialize();

    // Programa OpenGL utilizado para dibujar profundidades.
    GLuint m_depthProgram = 0;

    // Ubicaciones de los uniforms del shader de profundidad.
    GLint m_locLightViewProjection = -1;
    GLint m_locLightPosition = -1;
    GLint m_locFarPlane = -1;

    // Un cubemap por cada slot de sombras.
    std::array<PointShadowMap, MaxShadowLights> m_maps;

    // Para cada luz normal, indica que slot de sombras utiliza.
    // -1 significa que no proyecta sombras.
    std::array<int, MAX_LIGHTS> m_lightToSlot{};

    // Radio utilizado al generar cada cubemap.
    std::array<float, MaxShadowLights> m_farPlanes{};

    // Cantidad de slots utilizados en el frame actual.
    int m_activeCount = 0;

    // Interruptor global.
    bool m_enabled = true;

    // Indica si el shader de profundidad esta preparado.
    bool m_initialized = false;

    // Restablecer las asociaciones de luces del frame.
    void clearFrameAssignments();
};
