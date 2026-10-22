#include "TextureLibrary.h"

// SOIL convierte cualquier imagen a RGBA: 4 bytes por pixel, asi las filas nunca quedan desalineadas
constexpr int RGBA_CHANNELS = 4;

static GLuint uploadRgbaTexture(const unsigned char* pixels, int width, int height) {
    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    return texture;
}

void TextureLibrary::init() {
    const unsigned char whitePixel[RGBA_CHANNELS] = { 255, 255, 255, 255 };
    w_white = uploadRgbaTexture(whitePixel, 1, 1);
}

GLuint TextureLibrary::load(const std::filesystem::path& file) {
    const std::string key = file.lexically_normal().string();
    std::unordered_map<std::string, GLuint>::const_iterator alreadyLoaded = w_byPath.find(key);
    if (alreadyLoaded != w_byPath.end()) return alreadyLoaded->second;

    int width = 0, height = 0, channelsInFile = 0;
    unsigned char* pixels = SOIL_load_image(key.c_str(), &width, &height, &channelsInFile, SOIL_LOAD_RGBA);
    if (!pixels) {
        std::cerr << "[Textura] no se puede cargar " << key << ": " << SOIL_last_result() << " (se usa blanco)\n";
        return w_white;
    }

    const GLuint texture = uploadRgbaTexture(pixels, width, height);
    SOIL_free_image_data(pixels);
    w_byPath[key] = texture;
    return texture;
}

void TextureLibrary::destroy() {
    for (const std::pair<const std::string, GLuint>& entry : w_byPath) glDeleteTextures(1, &entry.second);
    glDeleteTextures(1, &w_white);
    w_byPath.clear();
    w_white = 0;
}