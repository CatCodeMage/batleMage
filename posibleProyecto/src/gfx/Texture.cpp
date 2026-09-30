#include "gfx/Texture.h"

#include <algorithm>
#include <vector>

namespace gfx {

Texture Texture::checker(int size, int cells, const glm::vec3& a, const glm::vec3& b) {
    std::vector<unsigned char> data(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 3u);

    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int cx = x * cells / size;
            const int cy = y * cells / size;
            glm::vec3 c = ((cx + cy) & 1) ? a : b;

            // Ruido determinista por pixel (para que no se vea plano)
            const unsigned int h = static_cast<unsigned int>(x) * 73856093u ^ static_cast<unsigned int>(y) * 19349663u;
            const float n = static_cast<float>((h >> 8) & 0xFFu) / 255.0f - 0.5f;
            c += glm::vec3(n * 0.06f);

            // Juntas oscuras entre baldosas
            if ((x * cells) % size < cells || (y * cells) % size < cells) c *= 0.7f;

            const std::size_t o = (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)) * 3u;
            for (int k = 0; k < 3; ++k) data[o + static_cast<std::size_t>(k)] = static_cast<unsigned char>(std::clamp(c[k], 0.0f, 1.0f) * 255.0f);
        }
    }

    GLuint id = 0;
    glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, size, size, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
    return Texture(id);
}

Texture::~Texture() {
    if (id_) glDeleteTextures(1, &id_);
}

Texture::Texture(Texture&& o) noexcept : id_(o.id_) {
    o.id_ = 0;
}

Texture& Texture::operator=(Texture&& o) noexcept {
    if (this != &o) {
        if (id_) glDeleteTextures(1, &id_);
        id_ = o.id_;
        o.id_ = 0;
    }
    return *this;
}

void Texture::bind(int unit) const {
    glActiveTexture(GL_TEXTURE0 + static_cast<GLenum>(unit));
    glBindTexture(GL_TEXTURE_2D, id_);
}

}  // namespace gfx
