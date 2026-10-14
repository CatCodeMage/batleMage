#pragma once
#include "global.h"

// Recursos de una sola luz puntual: un cubemap de profundidad y un FBO.
// init/destroy deben ejecutarse mientras el contexto OpenGL siga vivo.
class ShadowMap {
public:
    static constexpr int DEFAULT_RESOLUTION = 512;

    ShadowMap() = default;
    ShadowMap(const ShadowMap&) = delete;
    ShadowMap& operator=(const ShadowMap&) = delete;

    bool init(int resolution = DEFAULT_RESOLUTION);
    void destroy();
    bool ready() const { return w_framebuffer != 0; }
    int resolution() const { return w_resolution; }
    GLuint texture() const { return w_depthCube; }
    // Selecciona una cara y limpia su profundidad; no dibuja las mallas.
    void beginFace(unsigned face) const;

private:
    GLuint w_framebuffer = 0;
    GLuint w_depthCube = 0;
    int w_resolution = 0;
};
