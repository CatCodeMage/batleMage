
#include "ShadowSystem.h"
#include "shaders.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace {

// Vertex shader de profundidad.
// Utiliza los atributos actuales de Mesh.cpp:
// location 0 = posicion
// location 3..5 = matriz de modelo por instancia
constexpr const char* DepthVertexSource = R"GLSL(
#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 3) in mat4 iModel;

uniform mat4 uLightViewProjection;

out vec3 vWorldPos;

void main() {
    vec4 worldPos = iModel * vec4(aPos, 1.0);

    vWorldPos = worldPos.xyz;

    gl_Position = uLightViewProjection * worldPos;
}
)GLSL";

// Fragment shader de profundidad.
// Guarda la distancia radial desde la luz, normalizada.
constexpr const char* DepthFragmentSource = R"GLSL(
#version 330 core

in vec3 vWorldPos;

uniform vec3 uLightPosition;
uniform float uFarPlane;

void main() {
    float distanceToLight =
        length(vWorldPos - uLightPosition);

    gl_FragDepth = clamp(
        distanceToLight / uFarPlane,
        0.0,
        1.0
    );
}
)GLSL";

// Comprobar si un shader se ha compilado.
bool shaderCompiled(GLuint shader) {
    if (shader == 0) {
        return false;
    }

    GLint compiled = GL_FALSE;

    glGetShaderiv(
        shader,
        GL_COMPILE_STATUS,
        &compiled
    );

    return compiled == GL_TRUE;
}

} // namespace


// ---------------------------------------------------------
// Constructor
// ---------------------------------------------------------

ShadowSystem::ShadowSystem() {
    clearFrameAssignments();
}


// ---------------------------------------------------------
// Destructor
// ---------------------------------------------------------

ShadowSystem::~ShadowSystem() {
    destroy();
}


// ---------------------------------------------------------
// Limpiar asociaciones del frame
// ---------------------------------------------------------

void ShadowSystem::clearFrameAssignments() {
    // Inicialmente ninguna luz tiene un mapa asignado.
    m_lightToSlot.fill(-1);

    // Valores por defecto para los radios.
    m_farPlanes.fill(1.0f);

    // Ningun slot esta ocupado en este frame.
    m_activeCount = 0;
}


// ---------------------------------------------------------
// Inicializacion
// ---------------------------------------------------------

bool ShadowSystem::initialize() {
    if (m_initialized) {
        return true;
    }

    // Comprobar unidades de textura disponibles.
    GLint textureUnits = 0;

    glGetIntegerv(
        GL_MAX_TEXTURE_IMAGE_UNITS,
        &textureUnits
    );

    if (textureUnits < FirstTextureUnit + MaxShadowLights)
    {
        std::cerr
            << "ShadowSystem: no hay suficientes "
            << "unidades de textura.\n";

        return false;
    }

    // Compilar el vertex shader de profundidad.
    GLuint vs = compileShader(
        GL_VERTEX_SHADER,
        DepthVertexSource
    );

    // Compilar el fragment shader de profundidad.
    GLuint fs = compileShader(
        GL_FRAGMENT_SHADER,
        DepthFragmentSource
    );

    // Comprobar compilacion.
    if (!shaderCompiled(vs) || !shaderCompiled(fs))
    {
        if (vs != 0) {
            glDeleteShader(vs);
        }

        if (fs != 0) {
            glDeleteShader(fs);
        }

        return false;
    }

    // Crear el programa de profundidad.
    GLuint program = glCreateProgram();

    glAttachShader(program, vs);
    glAttachShader(program, fs);

    glLinkProgram(program);

    // Los shaders individuales ya no se necesitan.
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Comprobar que el programa se ha enlazado.
    GLint linked = GL_FALSE;

    glGetProgramiv(
        program,
        GL_LINK_STATUS,
        &linked
    );

    if (linked != GL_TRUE)
    {
        GLint logLength = 0;

        glGetProgramiv(
            program,
            GL_INFO_LOG_LENGTH,
            &logLength
        );

        std::string log(
            std::max(1, logLength),
            '\0'
        );

        glGetProgramInfoLog(
            program,
            GLsizei(log.size()),
            nullptr,
            log.data()
        );

        std::cerr
            << "ShadowSystem: error enlazando "
            << "el shader de profundidad:\n"
            << log << '\n';

        glDeleteProgram(program);

        return false;
    }

    m_depthProgram = program;

    // Buscar uniforms del shader de profundidad.
    m_locLightViewProjection = glGetUniformLocation(
        m_depthProgram,
        "uLightViewProjection"
    );

    m_locLightPosition = glGetUniformLocation(
        m_depthProgram,
        "uLightPosition"
    );

    m_locFarPlane = glGetUniformLocation(
        m_depthProgram,
        "uFarPlane"
    );

    m_initialized = true;

    return true;
}


// ---------------------------------------------------------
// Activar / desactivar sombras globalmente
// ---------------------------------------------------------

void ShadowSystem::setEnabled(bool enabled) {
    m_enabled = enabled;

    if (!enabled) {
        clearFrameAssignments();
    }
}


// ---------------------------------------------------------
// Renderizar los mapas de sombras
// ---------------------------------------------------------

void ShadowSystem::render(
    const std::vector<SceneLight>& lights,
    const DrawGeometry& drawGeometry
) {
    // Las asignaciones se recalculan cada frame.
    clearFrameAssignments();

    // Si las sombras estan desactivadas, no hacemos nada.
    if (!m_enabled) {
        return;
    }

    // Comprobar que todo esta preparado.
    if (!m_initialized || !drawGeometry) {
        return;
    }

    // Recordar el programa OpenGL anterior.
    GLint previousProgram = 0;

    glGetIntegerv(
        GL_CURRENT_PROGRAM,
        &previousProgram
    );

    // Activar el shader de profundidad.
    glUseProgram(m_depthProgram);

    // Renderer admite como maximo MAX_LIGHTS luces.
    const std::size_t lightCount = std::min(
        lights.size(),
        std::size_t(MAX_LIGHTS)
    );

    // Recorrer las luces de la escena.
    for (std::size_t lightIndex = 0; lightIndex < lightCount; lightIndex++)
    {

        // No podemos utilizar mas de cuatro cubemaps.
        if (m_activeCount >= MaxShadowLights) {
            break;
        }

        const SceneLight& sceneLight =
            lights[lightIndex];

        // Saltar las luces que no proyectan sombras.
        if (!sceneLight.castsShadow) {
            continue;
        }

        // Un radio no positivo no es valido.
        if (sceneLight.light.radius <= 0.0f) {
            continue;
        }

        // Elegir el siguiente slot libre.
        const int slot = m_activeCount;

        PointShadowMap& shadowMap =
            m_maps[slot];

        // Crear el cubemap si es la primera vez
        // que utilizamos este slot.
        if (!shadowMap.isInitialized()) {

            if (!shadowMap.initialize()) {
                std::cerr
                    << "ShadowSystem: error creando "
                    << "el cubemap de la luz "
                    << lightIndex << '\n';

                break;
            }
        }

        // Obtener los datos de la luz.
        const glm::vec3 position =
            sceneLight.light.pos;

        const float radius =
            sceneLight.light.radius;

        // Enviar la posicion al shader de profundidad.
        glUniform3fv(
            m_locLightPosition,
            1,
            glm::value_ptr(position)
        );

        // Enviar el radio de la luz.
        glUniform1f(
            m_locFarPlane,
            radius
        );

        // Generar las seis caras del cubemap.
        shadowMap.render(
            position,
            radius,

            [&](const glm::mat4& matrix) {

                // La matriz cambia para cada cara.
                glUniformMatrix4fv(
                    m_locLightViewProjection,
                    1,
                    GL_FALSE,
                    glm::value_ptr(matrix)
                );

                // Dibujar la geometria con el
                // shader de profundidad activo.
                drawGeometry();
            }
        );

        // Registrar que cubemap utiliza esta luz.
        m_lightToSlot[lightIndex] = slot;

        // Recordar su distancia maxima.
        m_farPlanes[slot] = radius;

        // El slot ya esta ocupado.
        ++m_activeCount;
    }

    // Recuperar el programa que estaba activo.
    glUseProgram(GLuint(previousProgram));
}


// ---------------------------------------------------------
// Conectar cubemaps con el shader principal
// ---------------------------------------------------------

void ShadowSystem::bindForLighting(GLuint lightingProgram) const
{
    if (lightingProgram == 0) {
        return;
    }

    // Activar el programa de iluminacion.
    glUseProgram(lightingProgram);

    // Estos uniforms estaran en el fragment shader.
    const GLint enabledLoc = glGetUniformLocation(
        lightingProgram,
        "uShadowsEnabled"
    );

    const GLint slotsLoc = glGetUniformLocation(
        lightingProgram,
        "uShadowSlotForLight[0]"
    );

    const GLint farLoc = glGetUniformLocation(
        lightingProgram,
        "uShadowFarPlanes[0]"
    );

    // Activacion global.
    if (enabledLoc >= 0) {
        glUniform1i(
            enabledLoc,
            (m_enabled && m_activeCount > 0) ? 1 : 0
        );
    }

    // Asociacion luz -> slot de sombra.
    if (slotsLoc >= 0) {
        glUniform1iv(
            slotsLoc,
            MAX_LIGHTS,
            m_lightToSlot.data()
        );
    }

    // Radios de las luces que generan sombras.
    if (farLoc >= 0) {
        glUniform1fv(
            farLoc,
            MaxShadowLights,
            m_farPlanes.data()
        );
    }

    // Guardar la unidad de textura activa.
    GLint previousActiveTexture = 0;

    glGetIntegerv(
        GL_ACTIVE_TEXTURE,
        &previousActiveTexture
    );

    // Enlazar los cuatro posibles cubemaps.
    for (int slot = 0;
         slot < MaxShadowLights;
         ++slot) {

        const int unit = FirstTextureUnit + slot;

        glActiveTexture(
            GL_TEXTURE0 + unit
        );

        // Enlazar solo los cubemaps que estan activos.
        glBindTexture(
            GL_TEXTURE_CUBE_MAP,
            slot < m_activeCount
                ? m_maps[slot].depthCubemap()
                : 0
        );

        // Nombre del sampler:
        // uShadowMap0, uShadowMap1, ...
        const std::string name =
            "uShadowMap" + std::to_string(slot);

        const GLint samplerLoc =
            glGetUniformLocation(
                lightingProgram,
                name.c_str()
            );

        if (samplerLoc >= 0) {
            glUniform1i(
                samplerLoc,
                unit
            );
        }
    }

    // Recuperar la unidad de textura anterior.
    glActiveTexture(
        GLenum(previousActiveTexture)
    );
}


// ---------------------------------------------------------
// Destruir recursos
// ---------------------------------------------------------

void ShadowSystem::destroy()
{
    clearFrameAssignments();

    // Liberar los cuatro cubemaps, si existen.
    for (PointShadowMap& map : m_maps) {
        map.destroy();
    }

    // Liberar el shader de profundidad.
    if (m_depthProgram != 0) {
        glDeleteProgram(m_depthProgram);

        m_depthProgram = 0;
    }

    m_locLightViewProjection = -1;
    m_locLightPosition = -1;
    m_locFarPlane = -1;

    m_initialized = false;
}
