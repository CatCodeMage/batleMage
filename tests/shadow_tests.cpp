#include "ShadowMath.h"
#include "ShadowMap.h"
#include "Renderer.h"
#include "shaders.h"
#include "cubo.h"
#include "suelo.h"
#include <limits>
#include <stdexcept>
#include <string>
#include <fstream>

// Sin framework: un fallo devuelve exit code 1 incluso al compilar con NDEBUG.
void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void testMatrices() {
    const glm::vec3 light(3.0f, -2.0f, 5.0f);
    const glm::vec3 axes[] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    const auto matrices = ShadowMath::faceMatrices(light, 8.0f);
    for (unsigned face = 0; face < 6; ++face) {
        glm::vec4 center = matrices[face] * glm::vec4(light + axes[face] * 2.0f, 1.0f);
        glm::vec3 ndc = glm::vec3(center) / center.w;
        check(center.w > 0.0f && std::abs(ndc.x) < 0.0001f && std::abs(ndc.y) < 0.0001f,
            "Cada eje debe quedar en el centro de su cara, delante de la camara");
        check(ndc.z > -1.0f && ndc.z < 1.0f, "El centro debe estar dentro del volumen visible");
        glm::vec4 far = matrices[face] * glm::vec4(light + axes[face] * 8.0f, 1.0f);
        check(std::abs(far.z / far.w - 1.0f) < 0.0001f, "El alcance debe coincidir con el plano lejano");
    }
    check(ShadowMath::validLight(light, 8.0f), "Luz valida rechazada");
    check(!ShadowMath::validLight(light, 0.0f), "Alcance cero aceptado");
    check(!ShadowMath::validLight(light, ShadowMath::NEAR_PLANE), "Alcance sin volumen aceptado");
    check(!ShadowMath::validLight(light, std::numeric_limits<float>::infinity()), "Alcance infinito aceptado");
    check(!ShadowMath::validLight(glm::vec3(std::numeric_limits<float>::quiet_NaN()), 8.0f),
        "Posicion NaN aceptada");
    std::cout << "PASS: matrices de las seis caras y limites de la luz\n";
}

void testDepthFaces(const MeshLibrary& library, MeshId cubeId) {
    ShadowMap map;
    check(!map.init(0), "Resolucion cero aceptada");
    check(map.init(128), "FBO de profundidad incompleto");
    GLuint program = createShaderProgram(shadowVertexSrc, shadowFragmentSrc);
    check(program != 0, "Shaders de profundidad invalidos");
    glUseProgram(program);
    glUniform3f(glGetUniformLocation(program, "uLightPos"), 0, 0, 0);
    glUniform1f(glGetUniformLocation(program, "uFarPlane"), 8.0f);

    const glm::vec3 axes[] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    std::vector<InstanceData> instances;
    for (const auto& axis : axes)
        instances.push_back({glm::translate(glm::mat4(1), axis * 2.0f), glm::vec4(1), 0});
    const Mesh& cube = library.get(cubeId);
    cube.uploadInstances(instances);
    const auto matrices = ShadowMath::faceMatrices(glm::vec3(0), 8.0f);
    for (unsigned face = 0; face < 6; ++face) {
        map.beginFace(face);
        glUniformMatrix4fv(glGetUniformLocation(program, "uLightViewProj"), 1,
            GL_FALSE, glm::value_ptr(matrices[face]));
        cube.drawUploadedInstances((GLsizei)instances.size());
        GLint drawFramebuffer = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFramebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, drawFramebuffer);
        float depth = 1.0f;
        glReadPixels(64, 64, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        check(std::abs(depth - 1.5f / 8.0f) < 0.003f,
            "La cara debe guardar la distancia al cubo, no la profundidad proyectada");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    map.destroy();
    map.destroy(); // La liberacion repetida debe ser segura.
    check(!map.ready(), "Recursos no liberados");
    glDeleteProgram(program);
    check(glGetError() == GL_NO_ERROR, "Error OpenGL al renderizar las seis caras");
    std::cout << "PASS: profundidad radial en las seis caras y liberacion de recursos\n";
}

// FBO de color independiente de la pantalla: la prueba no depende de su resolucion.
struct TestTarget {
    GLuint framebuffer = 0, color = 0, depth = 0;
    TestTarget() {
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glGenTextures(1, &color);
        glBindTexture(GL_TEXTURE_2D, color);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color, 0);
        glGenRenderbuffers(1, &depth);
        glBindRenderbuffer(GL_RENDERBUFFER, depth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 256, 256);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
        check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "FBO de prueba incompleto");
    }
    ~TestTarget() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteRenderbuffers(1, &depth);
        glDeleteTextures(1, &color);
        glDeleteFramebuffers(1, &framebuffer);
    }
};

float readRed(float worldX) {
    int x = (int)((worldX + 4.0f) / 8.0f * 256);
    unsigned char pixel[4] = {};
    glReadPixels(x, 128, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    return pixel[0] / 255.0f;
}

std::array<unsigned char, 4> readPixel(float worldX) {
    int x = (int)((worldX + 4.0f) / 8.0f * 256);
    std::array<unsigned char, 4> pixel{};
    glReadPixels(x, 128, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel.data());
    return pixel;
}

void saveCapture(const std::string& directory, const char* name) {
    if (directory.empty()) return;
    std::vector<unsigned char> pixels(256 * 256 * 3);
    glReadPixels(0, 0, 256, 256, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream image(directory + "/" + name, std::ios::binary);
    check(image.good(), "No se pudo guardar la captura");
    image << "P6\n256 256\n255\n";
    // OpenGL lee desde abajo; PPM guarda desde arriba.
    for (int row = 255; row >= 0; --row)
        image.write((const char*)pixels.data() + row * 256 * 3, 256 * 3);
}

void testScene(MeshLibrary& library, MeshId cubeId, MeshId floorId, const std::string& captures) {
    GLuint program = createShaderProgram(vertexSrc, fragmentSrc);
    check(program != 0, "Shaders de color invalidos");
    Renderer renderer;
    renderer.init(program);
    check(renderer.shadowsReady(), "No se pudieron inicializar las sombras");
    TestTarget target;
    GLuint query = 0;
    glGenQueries(1, &query);
    const glm::mat4 vp = glm::ortho(-4.0f, 4.0f, -4.0f, 4.0f, 0.1f, 20.0f) *
        glm::lookAt(glm::vec3(0, 7, 0), glm::vec3(0), glm::vec3(0, 0, -1));
    auto render = [&](bool shadows, bool cube, bool light, bool blueLight = false) {
        renderer.setShadowsEnabled(shadows);
        glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
        glViewport(0, 0, 256, 256);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        renderer.submit(floorId, glm::mat4(1), glm::vec4(0.4f));
        if (cube) renderer.submit(cubeId, glm::translate(glm::mat4(1), glm::vec3(0, 1, 0)), glm::vec4(1));
        if (light) renderer.submitLight({glm::vec3(0, 4, 0), 8.0f, glm::vec3(1), 2.0f});
        if (blueLight) renderer.submitLight({glm::vec3(0, 4, 0), 8.0f, glm::vec3(0, 0, 1), 2.0f});
        // Comprueba que flush restaura la unidad de textura, FBOs y viewport.
        glActiveTexture(GL_TEXTURE3);
        glBeginQuery(GL_PRIMITIVES_GENERATED, query);
        renderer.flush(library, vp);
        glEndQuery(GL_PRIMITIVES_GENERATED);
        GLuint triangles = 0;
        glGetQueryObjectuiv(query, GL_QUERY_RESULT, &triangles);
        GLint draw = 0, read = 0, viewport[4], active = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
        glGetIntegerv(GL_VIEWPORT, viewport);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &active);
        check(draw == (GLint)target.framebuffer && read == (GLint)target.framebuffer,
            "La pasada de sombras no debe cambiar el FBO de la escena");
        check(viewport[0] == 0 && viewport[1] == 0 && viewport[2] == 256 && viewport[3] == 256,
            "La pasada de sombras no restauro el viewport");
        check(active == GL_TEXTURE3, "No se restauro la unidad de textura activa");
        check(glGetError() == GL_NO_ERROR, "Error OpenGL al dibujar la escena");
        return triangles;
    };

    check(render(false, true, true) == 14, "Sin sombras debe haber una sola pasada (14 triangulos)");
    saveCapture(captures, "shadows-off.ppm");
    float unshadowed = readRed(0.65f);
    float exposed = readRed(2.0f);
    check(render(true, true, true) == 98, "Con sombras debe haber siete pasadas (98 triangulos)");
    saveCapture(captures, "shadows-on.ppm");
    check(unshadowed - readRed(0.65f) > 0.1f, "El cubo no proyecta una sombra sobre el suelo");
    check(std::abs(exposed - readRed(2.0f)) < 0.025f, "El suelo expuesto se oscurecio indebidamente");

    render(true, false, true);
    check(std::abs(unshadowed - readRed(0.65f)) < 0.025f, "Quedan sombras antiguas al retirar el cubo");
    // Una sola muestra no detecta acne en zonas alejadas: compara todo el suelo.
    std::vector<unsigned char> floorShadowed(256 * 256 * 4), floorExposed(256 * 256 * 4);
    glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, floorShadowed.data());
    render(false, false, true);
    glReadPixels(0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, floorExposed.data());
    for (size_t pixel = 0; pixel < floorExposed.size(); ++pixel)
        check(std::abs((int)floorExposed[pixel] - (int)floorShadowed[pixel]) <= 3,
            "El suelo sin obstaculos no debe generar manchas de autosombreado");
    check(render(true, true, false) == 14, "Sin luces no debe renderizarse el mapa de sombras");
    check(readRed(0.65f) > 0.05f && readRed(0.65f) < unshadowed,
        "Sin luces debe permanecer la iluminacion ambiente");
    renderer.setShadowLightIndex(15); // Indice sin luz: debe omitir la pasada.
    render(true, true, true);
    check(std::abs(unshadowed - readRed(0.65f)) < 0.025f, "Indice ausente no desactivo las sombras");
    renderer.setShadowLightIndex(0);
    render(true, true, true);
    check(unshadowed - readRed(0.65f) > 0.1f, "No se pueden reactivar las sombras");

    // Comprueba que el indice tambien funciona con una segunda luz seleccionada.
    render(false, true, true, true);
    auto withoutShadow = readPixel(0.65f);
    renderer.setShadowLightIndex(1);
    render(true, true, true, true);
    auto blueShadow = readPixel(0.65f);
    check(std::abs((int)withoutShadow[0] - (int)blueShadow[0]) <= 2,
        "La sombra azul no debe reducir la contribucion roja de la otra luz");
    check((int)withoutShadow[2] - (int)blueShadow[2] > 25,
        "La segunda luz no proyecta sombras al seleccionarla");

    renderer.destroy();
    renderer.destroy();
    glDeleteProgram(program);
    glDeleteQueries(1, &query);
    check(glGetError() == GL_NO_ERROR, "Error OpenGL al liberar el renderer");
    std::cout << "PASS: sombra visible, suelo iluminado, desactivacion, reactivacion y estado OpenGL\n";
    std::cout << "PASS: siete/una pasadas y sombra limitada a la luz seleccionada\n";
    std::cout << "PASS: suelo sin manchas en toda la imagen (256 x 256)\n";
}

int main(int argc, char** argv) {
    GLFWwindow* window = nullptr;
    try {
        testMatrices();
        if (argc > 1 && std::string(argv[1]) == "--math-only") return 0;
        check(glfwInit() == GLFW_TRUE, "No se pudo inicializar GLFW para las pruebas GPU");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        window = glfwCreateWindow(256, 256, "Shadow tests", nullptr, nullptr);
        check(window != nullptr, "No hay contexto OpenGL 3.3 para las pruebas GPU");
        glfwMakeContextCurrent(window);
        glewExperimental = GL_TRUE;
        check(glewInit() == GLEW_OK, "GLEW no pudo inicializarse");
        // Algunas versiones de GLEW dejan INVALID_ENUM al consultar extensiones en core.
        while (glGetError() != GL_NO_ERROR) {}
        std::cout << "GPU: " << glGetString(GL_RENDERER) << "\n";
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LESS);
        MeshLibrary library;
        MeshId cube = library.add("cube", std::vector<float>(std::begin(vertices), std::end(vertices)),
            std::vector<unsigned>(std::begin(indices), std::end(indices)));
        MeshId floor = library.add("floor", std::vector<float>(std::begin(floorVertices), std::end(floorVertices)),
            std::vector<unsigned>(std::begin(floorIndices), std::end(floorIndices)));
        testDepthFaces(library, cube);
        const std::string captures = argc > 2 && std::string(argv[1]) == "--capture-dir" ? argv[2] : "";
        testScene(library, cube, floor, captures);
        library.destroy();
        glfwDestroyWindow(window);
        glfwTerminate();
        std::cout << "Todas las pruebas han pasado.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << "\n";
        if (window) glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
}
