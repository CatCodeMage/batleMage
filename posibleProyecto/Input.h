#pragma once
#include "global.h"

class Input {
public:
    void init(GLFWwindow* window);
    bool keyDown(int key) const { return key >= 0 && key <= GLFW_KEY_LAST && w_keys[key]; }
    void consumeMouseDelta(float& dx, float& dy);   // devuelve lo acumulado y lo pone a 0
private:
    static void onKey(GLFWwindow* w, int key, int scancode, int action, int mods);
    static void onCursorPos(GLFWwindow* w, double x, double y);
    void toggleCursor(GLFWwindow* w);

    bool   w_keys[GLFW_KEY_LAST + 1] = {};
    bool   w_captured = false, w_firstMouse = true;
    double w_lastX = 0, w_lastY = 0;
    float  w_dx = 0, w_dy = 0;
};
