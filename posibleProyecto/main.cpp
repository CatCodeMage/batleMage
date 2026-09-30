#include <GL/glew.h>      // GLEW siempre antes que GLFW
#include <GLFW/glfw3.h>
#include <iostream>

int main()
{
    if (!glfwInit()) {
        std::cerr << "No se pudo inicializar GLFW\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Prueba", nullptr, nullptr);
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

    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
