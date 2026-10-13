#include "Renderer.h"
Renderer::Renderer() {
    // Para en caso de no hacer init que explote el codigo
    w_locModel = -1; 
    w_locViewProj = -1; 
    w_locColor = -1; 
    w_locKind = -1;
}

void Renderer::submit(MeshId id, const glm::mat4& model, const glm::vec4& color, int kind) {
    w_items.push_back({ id, model, color, kind });
}

void Renderer::init(GLuint program) {
    // Encontrar las variables de los shaders para tener el puntero para el flush
    // Los shaders son raros, pero mucho
    w_locModel = glGetUniformLocation(program, "uModel");
    w_locViewProj = glGetUniformLocation(program, "uViewProj");
    w_locColor = glGetUniformLocation(program, "uColor");
    w_locKind = glGetUniformLocation(program, "uKind");
}

void Renderer::flush(const MeshLibrary& lib, const glm::mat4& viewProj) {
    // Projeccio
    glUniformMatrix4fv(w_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));

    for (const DrawItem& it : w_items) {
        glUniformMatrix4fv(w_locModel, 1, GL_FALSE, glm::value_ptr(it.model)); // posicion + rotacion
        glUniform4fv(w_locColor, 1, glm::value_ptr(it.color)); // Color base
        glUniform1i(w_locKind, it.kind); // Tipo de draw del objeto, mirar shaders
        lib.get(it.mesh).draw(); // Dibuja el objeto
    }

    w_items.clear();
}