#include "app/Window.h"

#include <cstdio>
#include <stdexcept>

#include <GL/glew.h>   // GLEW siempre antes que GLFW
#include <GLFW/glfw3.h>

namespace app {

Window::Window(int width, int height, const char* title, bool vsync) {
    if (!glfwInit()) throw std::runtime_error("No se pudo inicializar GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);   // tamano fijo: la grabacion siempre sale a la misma resolucion
    glfwWindowHint(GLFW_SAMPLES, 4);

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        throw std::runtime_error("No se pudo crear la ventana (se necesita OpenGL 3.3)");
    }
    glfwMakeContextCurrent(window_);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window_);
        glfwTerminate();
        throw std::runtime_error("No se pudo inicializar GLEW");
    }
    glGetError();   // glewInit deja un GL_INVALID_ENUM inocuo con perfil core

    glfwSwapInterval(vsync ? 1 : 0);
    glEnable(GL_MULTISAMPLE);

    std::printf("OpenGL: %s\n", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

Window::~Window() {
    if (window_) glfwDestroyWindow(window_);
    glfwTerminate();
}

void Window::framebufferSize(int& w, int& h) const { glfwGetFramebufferSize(window_, &w, &h); }
bool Window::shouldClose() const { return glfwWindowShouldClose(window_) != 0; }
void Window::requestClose() { glfwSetWindowShouldClose(window_, GLFW_TRUE); }
void Window::swapBuffers() { glfwSwapBuffers(window_); }
void Window::pollEvents() { glfwPollEvents(); }
void Window::setTitle(const std::string& title) { glfwSetWindowTitle(window_, title.c_str()); }
double Window::time() const { return glfwGetTime(); }

}  // namespace app
