#include "Mesh.h"

void Mesh::create(const std::vector<float>& verts, const std::vector<unsigned>& idx) {
    // Set buffers
    glGenVertexArrays(1, &w_vao);
    glGenBuffers(1, &w_vbo);
    glGenBuffers(1, &w_ebo);

    glBindVertexArray(w_vao);
    glBindBuffer(GL_ARRAY_BUFFER, w_vbo);

    // Set model data
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

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
    w_ebo = w_vbo = w_vao = 0;
    w_indexCount = 0;
}