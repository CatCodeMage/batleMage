#include "Renderer.h"

Renderer::Renderer() {
    w_locViewProj = -1;
    w_locNumLights = -1;
    w_locLights = -1;
}

void Renderer::init(GLuint program) {
    w_locViewProj = glGetUniformLocation(program, "uViewProj");
    w_locNumLights = glGetUniformLocation(program, "uNumLights");
    w_locLights = glGetUniformLocation(program, "uLights[0]");
}

void Renderer::submitLight(const Light& light) {
    if (w_lights.size() >= MAX_LIGHTS) return;     // de momento se ignoran las que sobran
    w_lights.push_back(light);
}
 
void Renderer::submit(MeshId id, const glm::mat4& model, const glm::vec4& color, int kind) {
    assert(id != INVALID_MESH);
    if (id >= w_batches.size()) w_batches.resize(id + 1);
    w_batches[id].push_back({ model, color, (float)kind });
}

void Renderer::drawShadowGeometry(const MeshLibrary& lib) const {
    for (MeshId i = 0; i < w_batches.size(); ++i) {
        lib.get(i).drawInstances(w_batches[i]);
    }
}

void Renderer::flush(const MeshLibrary& lib, const glm::mat4& viewProj) {

    glUniformMatrix4fv(w_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));

    glUniform1i(w_locNumLights, (GLint)w_lights.size());
    if (!w_lights.empty())
        glUniform4fv(w_locLights, (GLsizei)(w_lights.size() * 2), (const GLfloat*)w_lights.data());



    for (MeshId i = 0; i < w_batches.size(); ++i) {
        lib.get(i).drawInstances(w_batches[i]);
        w_batches[i].clear();
    }

    w_lights.clear();
}