#pragma once
#include "global.h"

namespace bm::anim { struct ImportedTexture; }

// Carga cada textura una sola vez por ruta. La blanca 1x1 se usa cuando una pieza no tiene textura
// o no se pudo cargar: asi el shader siempre hace textura * color, sin casos especiales.

class TextureLibrary {
private:
    std::unordered_map<std::string, GLuint> w_byPath;
    GLuint w_white = 0;
public:
    void   init();                                    // necesita el contexto de OpenGL ya creado
    GLuint white() const { return w_white; }
    GLuint load(const std::filesystem::path& file);    
    GLuint load(const bm::anim::ImportedTexture& texture, const std::filesystem::path& modelFile);
    void   destroy();
};
