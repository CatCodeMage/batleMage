#pragma once
#include "global.h"

extern const char* vertexSrc;
extern const char* fragmentSrc;
extern const char* depthFragmentSrc;

GLuint createShaderProgram(const char* vertex, const char* fragment);

GLuint setShaders();
