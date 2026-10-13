#include "shaders.h"

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in mat4 iModel;     // ocupa las locations 2, 3, 4 y 5
layout (location = 6) in vec4 iColor;
layout (location = 7) in float iKind;
uniform mat4 uViewProj;
out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vColor;
flat out float vKind;
void main() {
    vec4 world = iModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(iModel) * aNormal;
    vColor = iColor;
    vKind = iKind;
    gl_Position = uViewProj * world;
})";

const char* fragmentSrc = R"(
#version 330 core
#define MAX_LIGHTS 16
in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vColor;
flat in float vKind;
uniform int  uNumLights;
uniform vec4 uLights[2 * MAX_LIGHTS];   // [2*i] = pos.xyz + alcance ; [2*i+1] = color.rgb + intensidad
out vec4 FragColor;
void main() {
    vec3 base;
    if (vKind > 0.5) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        base = vec3(mix(0.25, 0.6, c));
    } else {
        base = vColor.rgb;
    }
    vec3 n   = normalize(vNormal);
    // Luces ambiente
    vec3 sky    = vec3(0.45, 0.55, 0.75); 
    vec3 ground = vec3(0.12, 0.10, 0.08);
    vec3 lit    = mix(ground, sky, n.y * 0.5 + 0.5) * 0.6;
    

    for (int i = 0; i < uNumLights; ++i) {
        vec4  pr      = uLights[2 * i];
        vec4  ci      = uLights[2 * i + 1];
        vec3  toLight = pr.xyz - vWorldPos;
        float dist    = length(toLight);
        float diff    = max(dot(n, toLight / max(dist, 0.0001)), 0.0);
        float atten   = pow(clamp(1.0 - dist / pr.w, 0.0, 1.0), 2.0);
        lit += ci.rgb * ci.a * diff * atten;
    }
    FragColor = vec4(base * lit, 1.0);
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

