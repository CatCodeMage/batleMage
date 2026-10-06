#pragma once
#include "global.h"

extern const char* vertexSrc;
extern const char* fragmentSrc;

GLuint compileShader(GLenum type, const char* src);