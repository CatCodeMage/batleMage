#include "gfx/Recorder.h"

#include <cstdio>
#include <cstring>
#include <filesystem>

#include <GL/glew.h>
#include <soil2/SOIL2.h>

namespace gfx {

Recorder::Recorder(const std::string& directory, int width, int height)
    : dir_(directory),
      width_(width),
      height_(height),
      pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 3u),
      flipped_(pixels_.size()) {
    std::filesystem::create_directories(dir_);
}

bool Recorder::capture() {
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width_, height_, GL_RGB, GL_UNSIGNED_BYTE, pixels_.data());

    // OpenGL entrega la imagen de abajo a arriba; los PNG van de arriba a abajo.
    const std::size_t row = static_cast<std::size_t>(width_) * 3u;
    for (int y = 0; y < height_; ++y) {
        std::memcpy(flipped_.data() + static_cast<std::size_t>(y) * row,
                    pixels_.data() + static_cast<std::size_t>(height_ - 1 - y) * row, row);
    }

    char name[512];
    std::snprintf(name, sizeof(name), "%s/frame_%05d.png", dir_.c_str(), frames_);
    const int ok = SOIL_save_image(name, SOIL_SAVE_TYPE_PNG, width_, height_, 3, flipped_.data());
    if (ok) ++frames_;
    return ok != 0;
}

}  // namespace gfx
