#include "shaders.h"

const char* vertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;
layout (location = 3) in mat4 iModel;     // ocupa las locations 3, 4, 5 y 6
layout (location = 7) in vec4 iColor;
layout (location = 8) in float iKind;
uniform mat4 uViewProj;
out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec4 vColor;
flat out float vKind;
void main() {
    vec4 world = iModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    vNormal = mat3(iModel) * aNormal;
    vUV = aUV;
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
in vec2 vUV;
flat in float vKind;
uniform sampler2D uAlbedo;
uniform int  uNumLights;
uniform vec4 uLights[2 * MAX_LIGHTS];   // [2*i] = pos.xyz + alcance ; [2*i+1] = color.rgb + intensidad

//Uniforms de sombras
uniform int uShadowsEnabled;
uniform int uShadowSlotForLight[MAX_LIGHTS];
uniform float uShadowFarPlanes[4];
uniform samplerCube uShadowMap0;
uniform samplerCube uShadowMap1;
uniform samplerCube uShadowMap2;
uniform samplerCube uShadowMap3;

out vec4 FragColor;

// Leer la profundidad del cubemap correspondiente
float readShadowDepth(int slot, vec3 direction) {
    if (slot == 0) return texture(uShadowMap0, direction).r;
    if (slot == 1) return texture(uShadowMap1, direction).r;
    if (slot == 2) return texture(uShadowMap2, direction).r;
    if (slot == 3) return texture(uShadowMap3, direction).r;
    return 1.0;
}

// Calcular la visibilidad de una luz
float shadowVisibility(int lightIndex, vec3 lightPos, vec3 normal) {
    if (uShadowsEnabled == 0) return 1.0;

    int slot = uShadowSlotForLight[lightIndex];
    if (slot < 0 || slot >= 4) return 1.0;

    float farPlane = uShadowFarPlanes[slot];
    if (farPlane <= 0.0) return 1.0;

    vec3 lightToFrag = vWorldPos - lightPos;
    float currentDepth = length(lightToFrag);
    if (currentDepth >= farPlane) return 1.0;

    vec3 directionToLight = -lightToFrag / max(currentDepth, 0.0001);
    float angle = max(dot(normal, directionToLight), 0.0);
    float bias = max(0.01 * (1.0 - angle), 0.005);

	float storedDistance = readShadowDepth(slot, lightToFrag) * farPlane;

	if (currentDepth - bias > storedDistance)
		return 0.0;

	return 1.0;
}

void main() {
    vec3 base;
    if (vKind > 0.5) {
        float c = mod(floor(vWorldPos.x) + floor(vWorldPos.z), 2.0);
        base = vec3(mix(0.25, 0.6, c));
     } else {
        base = texture(uAlbedo, vUV).rgb * vColor.rgb;
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

        float visibility = 1.0;
        if (diff > 0.0 && atten > 0.0)
            visibility = shadowVisibility(i, pr.xyz, n);

        lit += ci.rgb * ci.a * diff * atten * visibility;
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

