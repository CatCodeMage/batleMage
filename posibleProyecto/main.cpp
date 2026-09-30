#include <GL/glew.h>      // GLEW debe ir ANTES que GLFW
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <iostream>

int main()
{
    glm::vec3 v(1.0f, 2.0f, 3.0f);
    std::cout << "Includes OK: " << v.x << ", " << v.y << ", " << v.z << "\n";
    return 0;
}