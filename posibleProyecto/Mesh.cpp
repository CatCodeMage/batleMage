#include "Mesh.h"

void Mesh::create(const std::vector<float>& verts, const std::vector<unsigned>& idx) {
    // Crear los objetos de OpenGL
    glGenVertexArrays(1, &w_vao);
    glGenBuffers(1, &w_vbo);
    glGenBuffers(1, &w_ebo);
    glGenBuffers(1, &w_instanceVbo);

    glBindVertexArray(w_vao);

    // --- Datos por vértice: posición (3) + normal (3) ---
    glBindBuffer(GL_ARRAY_BUFFER, w_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    const GLsizei stride = 6 * sizeof(float);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // --- Datos por instancia: modelo (mat4), color (vec4), kind (float) ---
    // Este bind tiene que ir ANTES de los glVertexAttribPointer de abajo:
    // cada uno recuerda el buffer que esté enlazado en ese momento.
    glBindBuffer(GL_ARRAY_BUFFER, w_instanceVbo);

    for (int i = 0; i < 4; ++i) {                       // un mat4 son 4 columnas vec4
        glEnableVertexAttribArray(2 + i);                
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
            (void*)(sizeof(glm::vec4) * i));
        glVertexAttribDivisor(2 + i, 1);                // avanza por instancia, no por vértice
    }

    glEnableVertexAttribArray(6);                       // color
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(6, 1);

    glEnableVertexAttribArray(7);                       // kind
    glVertexAttribPointer(7, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, kind));
    glVertexAttribDivisor(7, 1);

    // --- Índices ---
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, w_ebo);
    w_indexCount = (GLsizei)idx.size();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, idx.size() * sizeof(unsigned), idx.data(), GL_STATIC_DRAW);
}

void Mesh::draw() const {
    glBindVertexArray(w_vao);
    glDrawElements(GL_TRIANGLES, w_indexCount, GL_UNSIGNED_INT, 0);
}

void Mesh::destroy() {
    glDeleteBuffers(1, &w_ebo);
    glDeleteBuffers(1, &w_vbo);
    glDeleteVertexArrays(1, &w_vao);
    glDeleteBuffers(1, &w_instanceVbo);
    w_ebo = w_vbo = w_vao = w_instanceVbo = 0;
    w_indexCount = 0;
}

void Mesh::drawInstances(const std::vector<InstanceData>& inst) const {
    uploadInstances(inst);
    drawUploadedInstances((GLsizei)inst.size());
}

void Mesh::uploadInstances(const std::vector<InstanceData>& inst) const {
    if (inst.empty()) return;
    glBindBuffer(GL_ARRAY_BUFFER, w_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, inst.size() * sizeof(InstanceData), inst.data(), GL_STREAM_DRAW);
}

void Mesh::drawUploadedInstances(GLsizei count) const {
    if (count <= 0) return;
    glBindVertexArray(w_vao);
    glDrawElementsInstanced(GL_TRIANGLES, w_indexCount, GL_UNSIGNED_INT, 0, count);
}
