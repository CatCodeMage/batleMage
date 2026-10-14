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
    vNormal = transpose(inverse(mat3(iModel))) * aNormal;
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
uniform samplerCube uShadowMap;
uniform int uShadowLight;             // -1 = sin sombras; indice de la luz que las proyecta
uniform float uShadowFar;
uniform float uShadowTexelSize;
out vec4 FragColor;

// Porcentaje visible (PCF): cinco comparaciones de distancia, no de color.
float shadowVisibility(vec3 fromLight, float distanceToLight, float diffuse) {
    if (distanceToLight <= 0.1 || distanceToLight >= uShadowFar) return 1.0;
    vec3 direction = fromLight / distanceToLight;
    vec3 helper = abs(direction.y) < 0.99 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    vec3 tangent = normalize(cross(direction, helper));
    vec3 bitangent = cross(direction, tangent);
    float stepSize = 2.0 * uShadowTexelSize;
    vec3 offsets[5] = vec3[5](vec3(0), tangent, -tangent, bitangent, -bitangent);
    // Los vecinos PCF cubren mas superficie lejos de la luz y a angulos rasantes.
    // El bias sigue ese tamano de texel para evitar manchas en el propio suelo.
    float worldTexel = 2.0 * distanceToLight * uShadowTexelSize;
    float slope = sqrt(max(1.0 - diffuse * diffuse, 0.0)) / max(diffuse, 0.1);
    float bias = max(0.01, 2.0 * worldTexel * (1.0 + slope));
    float visible = 0.0;
    for (int sampleIndex = 0; sampleIndex < 5; ++sampleIndex) {
        float closest = texture(uShadowMap, direction + offsets[sampleIndex] * stepSize).r * uShadowFar;
        visible += distanceToLight - bias <= closest ? 1.0 : 0.0;
    }
    return visible / 5.0;
}

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
        if (pr.w <= 0.0 || ci.a <= 0.0) continue;
        vec3  toLight = pr.xyz - vWorldPos;
        float dist    = length(toLight);
        float diff    = max(dot(n, toLight / max(dist, 0.0001)), 0.0);
        float atten   = pow(clamp(1.0 - dist / pr.w, 0.0, 1.0), 2.0);
        if (diff <= 0.0 || atten <= 0.0) continue;
        float visibility = i == uShadowLight ? shadowVisibility(-toLight, dist, diff) : 1.0;
        lit += ci.rgb * ci.a * diff * atten * visibility;
    }
    FragColor = vec4(base * lit, 1.0);
})";

// Solo posicion: comparte las mismas matrices de instancia del render normal.
const char* shadowVertexSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in mat4 iModel;
uniform mat4 uLightViewProj;
out vec3 vWorldPos;
void main() {
    vec4 world = iModel * vec4(aPos, 1.0);
    vWorldPos = world.xyz;
    gl_Position = uLightViewProj * world;
})";

const char* shadowFragmentSrc = R"(
#version 330 core
in vec3 vWorldPos;
uniform vec3 uLightPos;
uniform float uFarPlane;
void main() {
    // Distancia radial normalizada: la misma comparacion sirve para las seis caras.
    gl_FragDepth = length(vWorldPos - uLightPos) / uFarPlane;
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
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint createShaderProgram(const char* vertex, const char* fragment) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertex);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragment);
    if (!vs || !fs) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return 0;
    }
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint ok = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "Error de enlace de shaders:\n" << log << "\n";
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

