#pragma once
#include "cubo.h"

float vertices[] = {
    -0.5f, -0.5f,  0.5f,   // 0 delante, abajo-izquierda
     0.5f, -0.5f,  0.5f,   // 1 delante, abajo-derecha
     0.5f,  0.5f,  0.5f,   // 2 delante, arriba-derecha
    -0.5f,  0.5f,  0.5f,   // 3 delante, arriba-izquierda
    -0.5f, -0.5f, -0.5f,   // 4 detrás, abajo-izquierda
     0.5f, -0.5f, -0.5f,   // 5 detrás, abajo-derecha
     0.5f,  0.5f, -0.5f,   // 6 detrás, arriba-derecha
    -0.5f,  0.5f, -0.5f    // 7 detrás, arriba-izquierda
};

unsigned int indices[] = {
    0, 1, 2,  2, 3, 0,     // delante
    1, 5, 6,  6, 2, 1,     // derecha
    5, 4, 7,  7, 6, 5,     // detrás
    4, 0, 3,  3, 7, 4,     // izquierda
    3, 2, 6,  6, 7, 3,     // arriba
    4, 5, 1,  1, 0, 4      // abajo
};
