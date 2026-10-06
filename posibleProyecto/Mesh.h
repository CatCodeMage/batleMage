#pragma once
#include "global.h"

struct Mesh {
    GLuint w_vao = 0, w_vbo = 0, w_ebo = 0;
    GLsizei w_indexCount = 0;
     
    void create(const std::vector<float>& verts, const std::vector<unsigned>& idx);
    void draw() const;
    void destroy(); 
};