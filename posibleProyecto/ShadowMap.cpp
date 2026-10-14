#include "ShadowMap.h"

bool ShadowMap::init(int resolution) {
    destroy();
    GLint maxSize = 0;
    glGetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &maxSize);
    if (resolution <= 0 || resolution > maxSize) return false;

    // La inicializacion no debe cambiar el destino ni la textura del llamante.
    GLint previousDraw = 0, previousRead = 0, previousCube = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDraw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousRead);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousCube);

    w_resolution = resolution;
    glGenTextures(1, &w_depthCube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, w_depthCube);
    for (unsigned face = 0; face < 6; ++face)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_DEPTH_COMPONENT24,
            resolution, resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // Comparaciones manuales en el shader: NEAREST evita interpolar profundidades.
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &w_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, w_framebuffer);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, w_depthCube, 0);
    const bool complete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDraw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previousRead);
    glBindTexture(GL_TEXTURE_CUBE_MAP, previousCube);
    if (!complete) destroy();
    return complete;
}

void ShadowMap::beginFace(unsigned face) const {
    assert(ready() && face < 6);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, w_framebuffer);
    glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, w_depthCube, 0);
    glViewport(0, 0, w_resolution, w_resolution);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowMap::destroy() {
    glDeleteFramebuffers(1, &w_framebuffer);
    glDeleteTextures(1, &w_depthCube);
    w_framebuffer = w_depthCube = 0;
    w_resolution = 0;
}
