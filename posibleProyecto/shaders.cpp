#include "shaders.h"

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 uModel;
uniform mat4 uViewProj;
out vec3 vWorldPos;
void main() {
    vec4 world = uModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    gl_Position = uViewProj * world;
})";

const char* fragmentSrc = R"(
#version 330 core
in vec3 vWorldPos;
uniform vec4 uColor;
uniform int  uKind;          // 0 = liso, 1 = tablero
out vec4 FragColor;
void main() {
    if (uKind == 1) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        FragColor = vec4(vec3(mix(0.25, 0.6, c)), 1.0);
    } else {
        FragColor = uColor;
    }
})";


GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Error de shader:\n" << log << "\n";
    }
    return shader;
}

