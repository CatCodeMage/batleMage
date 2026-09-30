#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace gfx {

// Textura 2D con RAII.
class Texture {
public:
    // Textura procedural de tablero (con ruido y juntas). No depende de ningun archivo de imagen.
    static Texture checker(int size, int cells, const glm::vec3& a, const glm::vec3& b);

    ~Texture();
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    Texture(Texture&& o) noexcept;
    Texture& operator=(Texture&& o) noexcept;

    void bind(int unit) const;

private:
    explicit Texture(GLuint id) : id_(id) {}

    GLuint id_ = 0;
};

}  // namespace gfx
