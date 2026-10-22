#include "Mesh.h"

constexpr GLuint LOC_POSITION = 0;
constexpr GLuint LOC_NORMAL = 1;
constexpr GLuint LOC_UV = 2;
constexpr GLuint LOC_INSTANCE_MODEL = 3;   // mat4: ocupa 3, 4, 5 y 6
constexpr GLuint LOC_INSTANCE_COLOR = 7;
constexpr GLuint LOC_INSTANCE_KIND = 8;
constexpr int    MAT4_COLUMNS = 4;

void Mesh::create(const std::vector<float>& verts, const std::vector<unsigned>& idx) {
    // Crear los objetos de OpenGL
    glGenVertexArrays(1, &w_vao);
    glGenBuffers(1, &w_vbo);
    glGenBuffers(1, &w_ebo);
    glGenBuffers(1, &w_instanceVbo);

    glBindVertexArray(w_vao);

    // --- Datos por vertice: posicion (3) + normal (3) + uv (2) ---
    glBindBuffer(GL_ARRAY_BUFFER, w_vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    const GLsizei stride = FLOATS_PER_VERTEX * sizeof(float);
    glVertexAttribPointer(LOC_POSITION, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(LOC_POSITION);
    glVertexAttribPointer(LOC_NORMAL, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(LOC_NORMAL);
    glVertexAttribPointer(LOC_UV, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(LOC_UV);

    // --- Datos por instancia: modelo (mat4), color (vec4), kind (float) ---
    // Este bind tiene que ir ANTES de los glVertexAttribPointer de abajo:
    // cada uno recuerda el buffer que este enlazado en ese momento.
    glBindBuffer(GL_ARRAY_BUFFER, w_instanceVbo);

    // Guarda los InstanceData dentro del buffer
    for (int column = 0; column < MAT4_COLUMNS; ++column) {
        const GLuint location = LOC_INSTANCE_MODEL + column;
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
            (void*)(sizeof(glm::vec4) * column));
        glVertexAttribDivisor(location, 1);              // avanza por instancia 
    }

    // Explicamos como leer color y donde esta el siguiente,
    glEnableVertexAttribArray(LOC_INSTANCE_COLOR);
    glVertexAttribPointer(LOC_INSTANCE_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, color));
    glVertexAttribDivisor(LOC_INSTANCE_COLOR, 1);

    // Explicamos como leer kind y donde esta el siguiente,
    glEnableVertexAttribArray(LOC_INSTANCE_KIND);
    glVertexAttribPointer(LOC_INSTANCE_KIND, 1, GL_FLOAT, GL_FALSE, sizeof(InstanceData),
        (void*)offsetof(InstanceData, kind));
    glVertexAttribDivisor(LOC_INSTANCE_KIND, 1);

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
    if (inst.empty()) return;
    glBindVertexArray(w_vao);
    glBindBuffer(GL_ARRAY_BUFFER, w_instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, inst.size() * sizeof(InstanceData), inst.data(), GL_STREAM_DRAW);
    glDrawElementsInstanced(GL_TRIANGLES, w_indexCount, GL_UNSIGNED_INT, 0, (GLsizei)inst.size());
}
