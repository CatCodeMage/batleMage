#pragma once
#include "global.h"

struct InstanceData {
    glm::mat4 model;
    glm::vec4 color;
    float     kind;
};

struct Mesh {
    GLuint w_vao = 0, w_vbo = 0, w_ebo = 0, w_instanceVbo = 0;
    GLsizei w_indexCount = 0;
     
    void create(const std::vector<float>& verts, const std::vector<unsigned>& idx);
    void draw() const;
    void destroy(); 
    void drawInstances(const std::vector<InstanceData>& inst) const;
    // Sube una vez por frame; reutiliza el buffer en las seis caras y en el color.
    void uploadInstances(const std::vector<InstanceData>& inst) const;
    void drawUploadedInstances(GLsizei count) const;
};
