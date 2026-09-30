#pragma once
#include <string>

struct GLFWwindow;

namespace app {

// Ventana GLFW + contexto OpenGL 3.3 core + GLEW inicializado. RAII: al destruirse lo libera todo.
class Window {
public:
    Window(int width, int height, const char* title, bool vsync);   // lanza std::runtime_error si falla
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    GLFWwindow* handle() const { return window_; }
    void framebufferSize(int& w, int& h) const;
    bool shouldClose() const;
    void requestClose();
    void swapBuffers();
    void pollEvents();
    void setTitle(const std::string& title);
    double time() const;

private:
    GLFWwindow* window_ = nullptr;
};

}  // namespace app
