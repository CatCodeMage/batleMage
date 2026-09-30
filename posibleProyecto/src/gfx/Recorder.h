#pragma once
#include <string>
#include <vector>

namespace gfx {

// Graba la imagen que hay ahora mismo en el framebuffer como una secuencia de PNG
// (frame_00000.png, frame_00001.png...). Se llama justo DESPUES de dibujar y ANTES de intercambiar buffers.
// Esas imagenes se pueden convertir a video con cualquier herramienta (p. ej. ffmpeg).
class Recorder {
public:
    Recorder(const std::string& directory, int width, int height);

    bool capture();
    int frames() const { return frames_; }
    const std::string& directory() const { return dir_; }

private:
    std::string dir_;
    int width_;
    int height_;
    int frames_ = 0;
    std::vector<unsigned char> pixels_;
    std::vector<unsigned char> flipped_;
};

}  // namespace gfx
