#include "global.h"
#include "shaders.h"
#include "cubo.h"
#include "systemInfo.h"
#include "camera.h"
#include "MeshLibrary.h"
#include "suelo.h"
#include "Input.h"
#include "Renderer.h"
#include "Transform.h"

int shouldContinue(GLFWwindow* window) {
    return glfwWindowShouldClose(window) == 0;
}

GLuint setShaders() { // Set shaders
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSrc);

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glEnable(GL_DEPTH_TEST); // Z-Buffer

    glUseProgram(program);

    return program;
}

void clearLastFrame() {
    glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void updateFrame(GLFWwindow* window) {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

void createMeshes(MeshLibrary& lib) {
    lib.add("cube",
        std::vector<float>(std::begin(vertices), std::end(vertices)),
        std::vector<unsigned>(std::begin(indices), std::end(indices)));

    lib.add("floor",
        std::vector<float>(std::begin(floorVertices), std::end(floorVertices)),
        std::vector<unsigned>(std::begin(floorIndices), std::end(floorIndices)));
}
 
void buclePrincipaLTiempoReal(GLFWwindow* window, SystemInfo& sysInfo) {
    
    MeshLibrary libMesh;    
    createMeshes(libMesh);
    MeshId cubeId = libMesh.find("cube");
    MeshId floorId = libMesh.find("floor");

    Camera viewCamera;

    GLuint program = setShaders();
    Renderer renderer;
    renderer.init(program);

    Input keysInput;

    keysInput.init(window);

    while (shouldContinue(window)) {
        clearLastFrame();

        sysInfo.getCurrentState(window);
        float time = sysInfo.getTimeStored();

        viewCamera.update(keysInput, sysInfo);

        glm::mat4 vp = viewCamera.get_proj() * viewCamera.get_view();

        Transform transformCube;
        transformCube.pos = glm::vec3(0.0f, 1.0f, 0.0f);
        transformCube.rot = glm::vec3(0.0f, time, 0.0f);
        renderer.submit(cubeId, transformCube.toMatrix(), glm::vec4(0.9f, 0.4f, 0.2f, 1.0f));

        renderer.submit(floorId, glm::mat4(1.0f), glm::vec4(1.0f), 1);

        renderer.submitLight(glm::vec3(3.0f * cos(time), 3.0f, 3.0f * sin(time)), 8.0f, glm::vec3(1.0f, 0.8f, 0.5f), 2.0f);
        renderer.submitLight(glm::vec3(-3.0f, 2.0f, 0.0f), 6.0f, glm::vec3(0.3f, 0.5f, 1.0f), 2.0f);

        renderer.flush(libMesh, vp);

        updateFrame(window);
    }

    libMesh.destroy();
}


int guranteWindowCreated(GLFWwindow* window) {
    if (!window) {
        std::cerr << "No se pudo crear la ventana\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "No se pudo inicializar GLEW\n";
        return -1;
    }

    std::cout << "OpenGL: " << glGetString(GL_VERSION) << "\n";
    return 0;
}

GLFWwindow* createWindow() {
    GLFWwindow* window = nullptr;

    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW\n"; 
    }
    else
    {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        window = glfwCreateWindow(mode->width, mode->height, "Prueba", monitor, nullptr);
    }

    return window;
}

int main()
{
    GLFWwindow* window = createWindow();

    SystemInfo sysInfo; // Se tiene que hacer despues de createWindow, respetar orden

    if (window == nullptr)
        return -1;
    
    sysInfo.getCurrentState(window);

    glViewport(0, 0, sysInfo.getWidht(), sysInfo.getHeight());

    if (guranteWindowCreated(window) != 0) 
        return -1;

    buclePrincipaLTiempoReal(window, sysInfo);

    glfwTerminate();

    return 0;
}
