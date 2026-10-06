#include "Input.h"

void Input::onKey(GLFWwindow* window, int key, int, int action, int) {
    Input* in = (Input*)glfwGetWindowUserPointer(window);
    if (key < 0 || key > GLFW_KEY_LAST) return;          // GLFW_KEY_UNKNOWN vale -1
    if (action == GLFW_PRESS)   in->w_keys[key] = true;

    if (action == GLFW_RELEASE) in->w_keys[key] = false; // REPEAT se ignora: sigue true

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, GLFW_TRUE);
        if (key == GLFW_KEY_H)      in->toggleCursor(window);
    }
}

void Input::onCursorPos(GLFWwindow* window, double x, double y) {
    Input* in = (Input*)glfwGetWindowUserPointer(window);
    if (!in->w_captured) return;                          // garantiza que el cursor es visible
    if (in->w_firstMouse) { in->w_lastX = x; in->w_lastY = y; in->w_firstMouse = false; }
    in->w_dx += (float)(x - in->w_lastX);
    in->w_dy += (float)(in->w_lastY - y);                 // invertido: en pantalla y crece hacia abajo
    in->w_lastX = x; in->w_lastY = y;
}

void Input::init(GLFWwindow* window) {
    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, onKey);
    glfwSetCursorPosCallback(window, onCursorPos);
}

void Input::consumeMouseDelta(float& dx, float& dy) {
    dx = w_dx;  dy = w_dy;     // entrega lo acumulado...
    w_dx = w_dy = 0.f;         // ...y lo pone a cero para el siguiente frame
}

void Input::toggleCursor(GLFWwindow* window) {
    w_captured = !w_captured;
    glfwSetInputMode(window, GLFW_CURSOR,
        w_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    w_firstMouse = true;      // la primera lectura tras el cambio no debe dar un salto
    w_dx = w_dy = 0.f;        // descarta lo acumulado antes del cambio
}