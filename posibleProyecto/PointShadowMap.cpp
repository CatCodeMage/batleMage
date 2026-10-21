
#include "PointShadowMap.h"

#include <algorithm>
#include <array>
#include <iostream>

PointShadowMap::~PointShadowMap() {
    destroy();
}

bool PointShadowMap::initialize() {
    if (isInitialized()) return true;

    //Guardamos los valores actuales del estado de OpenGL
    GLint previousTexture = 0;
    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;

    glGetIntegerv(
        GL_TEXTURE_BINDING_CUBE_MAP,
        &previousTexture
    );
    glGetIntegerv(
        GL_DRAW_FRAMEBUFFER_BINDING,
        &previousDrawFramebuffer
    );
    glGetIntegerv(
        GL_READ_FRAMEBUFFER_BINDING,
        &previousReadFramebuffer
    );

    //Crear la textura del cubemap
    glGenTextures(1, &m_depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, m_depthCubemap);

    //Reservar memoria para las 6 caras
    for (int face = 0; face < 6; ++face) {
        glTexImage2D(
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            0,
            GL_DEPTH_COMPONENT24,
            Resolution,
            Resolution,
            0,
            GL_DEPTH_COMPONENT,
            GL_FLOAT,
            nullptr
        );
    }

    //Configurar cómo se consulta la textura
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_MIN_FILTER,
        GL_NEAREST
    );
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_MAG_FILTER,
        GL_NEAREST
    );

    //Evitar problemas en los bordes
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_S,
        GL_CLAMP_TO_EDGE
    );
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_T,
        GL_CLAMP_TO_EDGE
    );
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_WRAP_R,
        GL_CLAMP_TO_EDGE
    );

    //Desactivar la comparación automática
    glTexParameteri(
        GL_TEXTURE_CUBE_MAP,
        GL_TEXTURE_COMPARE_MODE,
        GL_NONE
    );

    //Crear el framebuffer
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    //Asociar la primera cara del cubemap

    //Durante render(), cambiaremos el attachment para que apunte sucesivamente
    //a cada una de las seis caras. Así evitamos crear seis framebuffers independientes.

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X,
        m_depthCubemap,
        0
    );

    //Indicar que no renderizaremos colores
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    //OpenGL comprueba si la configuración del framebuffer es válida y utilizable.
    const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    
    //Restaurar el estado anterior de OpenGL
    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        GLuint(previousDrawFramebuffer)
    );
    glBindFramebuffer(
        GL_READ_FRAMEBUFFER,
        GLuint(previousReadFramebuffer)
    );
    glBindTexture(
        GL_TEXTURE_CUBE_MAP,
        GLuint(previousTexture)
    );

    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr
            << "PointShadowMap: framebuffer incompleto, codigo "
            << status << '\n';

        destroy();
        return false;
    }

    return true;
}

void PointShadowMap::render(
    const glm::vec3& lightPosition,
    float lightRadius,
    const DrawFace& drawFace
) {
    if (!isInitialized() || !drawFace ||
        lightRadius <= 0.0f) {
        return;
    }

    //Guardar el estado de Open GL
    GLint previousDrawFramebuffer = 0;
    GLint previousReadFramebuffer = 0;
    GLint previousViewport[4] = {};
    GLint previousDepthFunction = GL_LESS;
    GLboolean previousDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean previousCullFace = glIsEnabled(GL_CULL_FACE);
    GLint previousCullFaceMode = GL_BACK;
    glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);
    GLboolean previousDepthMask = GL_TRUE;
    GLdouble previousClearDepth = 1.0;

    glGetIntegerv(
        GL_DRAW_FRAMEBUFFER_BINDING,
        &previousDrawFramebuffer
    );
    glGetIntegerv(
        GL_READ_FRAMEBUFFER_BINDING,
        &previousReadFramebuffer
    );
    glGetIntegerv(GL_VIEWPORT, previousViewport);
    glGetIntegerv(
        GL_DEPTH_FUNC,
        &previousDepthFunction
    );
    glGetBooleanv(
        GL_DEPTH_WRITEMASK,
        &previousDepthMask
    );
    glGetDoublev(
        GL_DEPTH_CLEAR_VALUE,
        &previousClearDepth
    );

    //CREAR LA PROYECCION DE LA LUZ

    //nearPlane indica a partir de qué distancia comenzamos a rasterizar geometría.
    const float nearPlane = std::min(0.1f, lightRadius * 0.05f);

    //creamos la matriz (usamos 90º del campo de vision ya que hay 4 caras)
    //usamos perspectiva ya que una luz puntual emite hacia todas las direcciones
    const glm::mat4 projection = glm::perspective(
        glm::radians(90.0f),
        1.0f,
        nearPlane,
        lightRadius
    );

    //Definimos las direcciones de las camaras
    // +X, -X, +Y, -Y, +Z, -Z
    const std::array<glm::vec3, 6> directions = {
        glm::vec3( 1,  0,  0),
        glm::vec3(-1,  0,  0),
        glm::vec3( 0,  1,  0),
        glm::vec3( 0, -1,  0),
        glm::vec3( 0,  0,  1),
        glm::vec3( 0,  0, -1)
    };

    //Definimos los vectores up de las camaras
    const std::array<glm::vec3, 6> ups = {
        glm::vec3(0, -1,  0),
        glm::vec3(0, -1,  0),
        glm::vec3(0,  0,  1),
        glm::vec3(0,  0, -1),
        glm::vec3(0, -1,  0),
        glm::vec3(0, -1,  0)
    };

    //Activar el framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, Resolution, Resolution);

    //Configurar el depth test
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glClearDepth(1.0);

    // Evitar autoombrejat dibuixant las cares posteriors
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    //Por cada cara construimos la matriz de vista y el motor la dibuja
    for (int face = 0; face < 6; ++face) {
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
            m_depthCubemap,
            0
        );

        glClear(GL_DEPTH_BUFFER_BIT);

        const glm::mat4 view = glm::lookAt(
            lightPosition,
            lightPosition + directions[face],
            ups[face]
        );

        drawFace(projection * view);
    }

    //Restaurar el estado OpenGL
    glBindFramebuffer(
        GL_DRAW_FRAMEBUFFER,
        GLuint(previousDrawFramebuffer)
    );
    glBindFramebuffer(
        GL_READ_FRAMEBUFFER,
        GLuint(previousReadFramebuffer)
    );
    glViewport(
        previousViewport[0],
        previousViewport[1],
        previousViewport[2],
        previousViewport[3]
    );

    if (previousDepthTest) {
        glEnable(GL_DEPTH_TEST);
    } else {
        glDisable(GL_DEPTH_TEST);
    }

    glDepthMask(previousDepthMask);
    glDepthFunc(GLenum(previousDepthFunction));
    glClearDepth(previousClearDepth);

    if (previousCullFace) {
        glEnable(GL_CULL_FACE);
    }
    else {
        glDisable(GL_CULL_FACE);
    }

    glCullFace(GLenum(previousCullFaceMode));
}

//libera el framebuffer y el cubemap
void PointShadowMap::destroy() {
    if (m_framebuffer != 0) {
        glDeleteFramebuffers(1, &m_framebuffer);
        m_framebuffer = 0;
    }

    if (m_depthCubemap != 0) {
        glDeleteTextures(1, &m_depthCubemap);
        m_depthCubemap = 0;
    }
}
