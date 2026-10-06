#include "systemInfo.h"

void SystemInfo::getCurrentState(GLFWwindow* window) {
    w_time0 = w_time1;
    w_time1 = (float)glfwGetTime();
     
    glfwGetFramebufferSize(window, &w_widht, &w_height);
    if (w_height == 0) w_height = 1;   // ventana minimizada: evita dividir entre cero
}

SystemInfo::SystemInfo() {
    w_time0 = (float)glfwGetTime();
    w_time1 = (float)glfwGetTime(); 
}