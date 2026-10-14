#pragma once
#include "global.h"

extern const char* vertexSrc;
extern const char* fragmentSrc;
extern const char* shadowVertexSrc;
extern const char* shadowFragmentSrc;

GLuint compileShader(GLenum type, const char* src);
// Devuelve 0 si compilar o enlazar falla; nunca devuelve un programa incompleto.
GLuint createShaderProgram(const char* vertex, const char* fragment);
