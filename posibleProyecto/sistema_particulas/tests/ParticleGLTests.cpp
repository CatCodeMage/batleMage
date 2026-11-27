#include "../ParticleModule.h"
#include "../ParticleTestScene.h"
#include "../../shaders.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace bm::particles;

namespace {

constexpr int imageSize = 64;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void requireNoGLError(const char* stage) {
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) throw std::runtime_error(std::string(stage) + " GL error " + std::to_string(error));
}

GLint getInteger(GLenum name) {
    GLint value = 0;
    glGetIntegerv(name, &value);
    return value;
}

GLuint makeSolidTexture(const std::array<GLubyte, 4>& color) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    return texture;
}

std::array<GLubyte, 4> readPixel(int x = imageSize / 2, int y = imageSize / 2) {
    std::array<GLubyte, 4> pixel;
    glReadPixels(x, y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    return pixel;
}

float centerDepth() {
    float depth = 0.0f;
    glReadPixels(imageSize / 2, imageSize / 2, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
    requireNoGLError("reading depth");
    return depth;
}

void clearTarget(double depth) {
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(depth);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLAPIENTRY skipBufferUpload(GLenum, GLsizeiptr, const void*, GLenum) {}

int lateVertexArrayDeletes = 0;

void GLAPIENTRY countLateVertexArrayDeletes(GLsizei, const GLuint*) {
    ++lateVertexArrayDeletes;
}

void testShaderFailure() {
    try {
        const GLuint program = createShaderProgram(
            "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }",
            "#version 330 core\nsyntax error");
        glDeleteProgram(program);
    } catch (const std::runtime_error&) {
        requireNoGLError("rejected particle shader");
        return;
    }
    throw std::runtime_error("invalid shader should be rejected");
}

void testInitializationFailure(ParticleRendererGL& renderer, const ParticleMaterialTable& materials) {
    const auto bufferData = glBufferData;
    glBufferData = skipBufferUpload;
    try {
        renderer.initialize(materials);
    } catch (const std::runtime_error&) {
        glBufferData = bufferData;
        return;
    } catch (...) {
        glBufferData = bufferData;
        throw;
    }
    glBufferData = bufferData;
    throw std::runtime_error("missing quad storage should fail initialization");
}

void testRendering() {
    const GLuint white = makeSolidTexture({255, 255, 255, 255});
    const GLuint tinted = makeSolidTexture({128, 255, 255, 255});
    GLuint framebuffer, image, depthBuffer;
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &image);
    glGenRenderbuffers(1, &depthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glBindTexture(GL_TEXTURE_2D, image);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, imageSize, imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, image, 0);
    glBindRenderbuffer(GL_RENDERBUFFER, depthBuffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, imageSize, imageSize);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthBuffer);
    require(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "test framebuffer incomplete");
    glViewport(0, 0, imageSize, imageSize);

    const GLuint program = createShaderProgram(
        "#version 330 core\nvoid main() { gl_Position = vec4(0.0); }",
        "#version 330 core\nout vec4 color; void main() { color = vec4(1.0); }");
    GLuint vao, buffer, unpackBuffer, sampler;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &buffer);
    glGenBuffers(1, &unpackBuffer);
    glGenSamplers(1, &sampler);

    // Estado deliberadamente distinto al que requiere el pase de particulas.
    glUseProgram(program);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 7);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 8);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 160);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 3);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 2);
    glBindTexture(GL_TEXTURE_2D, white); // Unidad 0.
    glSamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindSampler(0, sampler);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, tinted);
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT_AND_BACK);
    glDisablei(GL_BLEND, 0);
    glEnablei(GL_BLEND, 1);
    glBlendFuncSeparate(GL_DST_COLOR, GL_SRC_COLOR, GL_ZERO, GL_ONE);
    glBlendEquationSeparate(GL_FUNC_REVERSE_SUBTRACT, GL_FUNC_SUBTRACT);
    requireNoGLError("test setup");

    const auto requireRestoredState = [&] {
        require(getInteger(GL_CURRENT_PROGRAM) == static_cast<GLint>(program), "program restoration");
        require(getInteger(GL_VERTEX_ARRAY_BINDING) == static_cast<GLint>(vao) &&
                getInteger(GL_ARRAY_BUFFER_BINDING) == static_cast<GLint>(buffer), "vertex binding restoration");
        GLint divisor = 0;
        glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_DIVISOR, &divisor);
        require(divisor == 7, "caller's instance attributes remain on its VAO");
        require(getInteger(GL_ACTIVE_TEXTURE) == GL_TEXTURE2 &&
                getInteger(GL_TEXTURE_BINDING_2D) == static_cast<GLint>(tinted), "active texture restoration");
        glActiveTexture(GL_TEXTURE0);
        require(getInteger(GL_TEXTURE_BINDING_2D) == static_cast<GLint>(white) &&
                getInteger(GL_SAMPLER_BINDING) == static_cast<GLint>(sampler), "texture and sampler restoration");
        glActiveTexture(GL_TEXTURE2);
        require(getInteger(GL_PIXEL_UNPACK_BUFFER_BINDING) == static_cast<GLint>(unpackBuffer) &&
                getInteger(GL_UNPACK_ALIGNMENT) == 8 && getInteger(GL_UNPACK_ROW_LENGTH) == 160 &&
                getInteger(GL_UNPACK_SKIP_ROWS) == 3 && getInteger(GL_UNPACK_SKIP_PIXELS) == 2, "pixel unpack restoration");
        require(!glIsEnabled(GL_DEPTH_TEST) && getInteger(GL_DEPTH_WRITEMASK) == GL_TRUE && glIsEnabled(GL_CULL_FACE),
                "depth and cull state restoration");
        require(!glIsEnabledi(GL_BLEND, 0) && glIsEnabledi(GL_BLEND, 1), "blend enable restoration");
        require(getInteger(GL_BLEND_SRC_RGB) == GL_DST_COLOR && getInteger(GL_BLEND_DST_RGB) == GL_SRC_COLOR &&
                getInteger(GL_BLEND_SRC_ALPHA) == GL_ZERO && getInteger(GL_BLEND_DST_ALPHA) == GL_ONE &&
                getInteger(GL_BLEND_EQUATION_RGB) == GL_FUNC_REVERSE_SUBTRACT &&
                getInteger(GL_BLEND_EQUATION_ALPHA) == GL_FUNC_SUBTRACT, "blend function restoration");
        requireNoGLError("state verification");
    };

    const ParticleStyleCatalog styles{{1, {.material = "white"}},
                                     {2, {.material = "tinted", .blend = ParticleBlend::Additive}}};
    const ParticleMaterialTable materials{{"white", white}, {"tinted", tinted}};
    ParticleRendererGL renderer;
    renderer.shutdown();
    testInitializationFailure(renderer, materials);
    requireRestoredState();
    renderer.initialize({{"white", tinted}, {"tinted", white}});
    renderer.initialize(materials);
    requireRestoredState();

    const std::array layers{
        Particle{.position = {0.0f, 0.0f, -0.4f}, .color = {0.0f, 1.0f, 0.0f, 0.5f}, .styleId = 1},
        Particle{.position = {0.0f, 0.0f, -0.8f}, .color = {1.0f, 0.0f, 0.0f, 0.5f}, .styleId = 2}
    };
    const glm::mat4 identity(1.0f);
    const auto orthographic = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 1.1f);
    clearTarget(1.0);
    const float clearedDepth = centerDepth();
    renderer.render(layers, styles, identity, orthographic);
    requireRestoredState();
    const auto blended = readPixel();
    require(std::abs(int(blended[0]) - 32) <= 2 && std::abs(int(blended[1]) - 128) <= 2 && blended[2] == 0,
            "texturing and ordered alpha/additive blending");
    require(clearedDepth > 0.99999f && centerDepth() == clearedDepth, "particles must not write depth");

    clearTarget(0.1);
    renderer.render(layers, styles, identity, orthographic);
    require(readPixel() == std::array<GLubyte, 4>{}, "particles hidden behind opaque depth");

    const std::array quads{
        Particle{.position = {-0.45f, 0.0f, -0.5f}, .color = {0.0f, 0.0f, 1.0f, 1.0f}, .size = 0.4f, .styleId = 1},
        Particle{.position = {0.45f, 0.0f, -0.5f}, .size = 0.4f, .rotation = glm::quarter_pi<float>(), .styleId = 1}
    };
    clearTarget(1.0);
    renderer.render(quads, styles, identity, orthographic);
    require(readPixel(18, 32)[2] == 255 && readPixel(46, 32)[0] == 255, "two instances in the same draw");
    require(readPixel(26, 32)[2] == 0 && readPixel(54, 32)[0] == 255 && readPixel(57, 32)[0] == 0,
            "billboard size and rotation");
    requireRestoredState();

    GLFWwindow* context = glfwGetCurrentContext();
    glfwMakeContextCurrent(nullptr);
    bool rejectedShutdown = false;
    try {
        renderer.shutdown();
    } catch (const std::runtime_error&) {
        rejectedShutdown = true;
    }
    glfwMakeContextCurrent(context);
    require(rejectedShutdown, "shutdown with live resources requires a current context");
    renderer.shutdown();
    renderer.shutdown();
    require(glIsTexture(white) && glIsTexture(tinted), "external textures still owned by caller");
    renderer.initialize(materials);
    renderer.shutdown();
    requireRestoredState();

    // Una rafaga real permite dibujar varias veces con camaras distintas en el mismo tick.
    ParticleModule module;
    module.initialize(presets::makeCatalog());
    module.setFrames(ParticleTestScene::makeFrames(ParticleTestScene::Scenario::Explosion), 60);
    module.seek(120);
    const auto born = module.particles();
    const std::vector<Particle> before(born.begin(), born.end());
    require(before.size() == 110, "burst ready to render");
    const auto projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 30.0f);
    const glm::vec3 target(0.0f, 1.0f, 0.0f);
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    for (glm::vec3 eye : {glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(5.0f, 1.0f, 0.0f)}) {
        clearTarget(1.0);
        const auto view = glm::lookAt(eye, target, up);
        for (int draw = 0; draw < 3; ++draw) module.render(view, projection);
        require(readPixel()[0] > 0, "fallback billboard visible from both cameras");
    }
    module.seek(120);
    const auto after = module.particles();
    require(after.size() == before.size(), "draws and repeated seek preserve burst count");
    for (std::size_t i = 0; i < before.size(); ++i) {
        const auto& a = before[i];
        const auto& b = after[i];
        require(a.position == b.position && a.velocity == b.velocity && a.color == b.color && a.size == b.size &&
                a.rotation == b.rotation && a.ageTicks == b.ageTicks && a.lifeTicks == b.lifeTicks && a.styleId == b.styleId,
                "draws preserve every simulated particle field");
    }
    module.shutdown();
    requireRestoredState();
    glUseProgram(0);
    glDeleteProgram(program);
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &buffer);
    glDeleteBuffers(1, &unpackBuffer);
    glDeleteSamplers(1, &sampler);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(1, &depthBuffer);
    const GLuint textures[] = {white, tinted, image};
    glDeleteTextures(3, textures);
    requireNoGLError("test cleanup");
}

}

int main() {
    if (!glfwInit()) {
        std::cerr << "Unable to initialize GLFW\n";
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(imageSize, imageSize, "Particle GL checks", nullptr, nullptr);
    if (!window) {
        const char* detail = nullptr;
        glfwGetError(&detail);
        std::cerr << "Unable to create OpenGL 3.3 Core context: " << (detail ? detail : "unknown") << '\n';
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    int result = 0;
    try {
        require(glewInit() == GLEW_OK, "unable to initialize GLEW");
        while (glGetError() != GL_NO_ERROR) {} // GLEW puede dejar un error al consultar extensiones Core.
        std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
        require((getInteger(GL_CONTEXT_PROFILE_MASK) & GL_CONTEXT_CORE_PROFILE_BIT) != 0, "Core context");
        testShaderFailure();
        testRendering();

        const auto deleteVertexArrays = glDeleteVertexArrays;
        {
            ParticleModule module;
            module.initialize({});
            glfwDestroyWindow(window);
            window = nullptr;
            glDeleteVertexArrays = countLateVertexArrayDeletes;
        }
        glDeleteVertexArrays = deleteVertexArrays;
        require(lateVertexArrayDeletes == 0, "module destruction after context must not delete GL resources");
        std::cout << "Particle OpenGL checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
