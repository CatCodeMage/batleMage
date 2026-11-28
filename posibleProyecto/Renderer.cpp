#include "Renderer.h"

constexpr GLint ALBEDO_TEXTURE_UNIT = 0;

Renderer::Renderer() {
    w_locViewProj = -1;
    m_locAlbedoColor = m_locEmissive = m_locRoughness = m_locMetallic = m_locOpacity = -1;
}

void Renderer::init(GLuint program) {
    w_program = program;
    w_locViewProj = glGetUniformLocation(program, "uViewProj");
    w_locCamPos = glGetUniformLocation(program, "uCamPos");
    glUniform1i(glGetUniformLocation(program, "uAlbedo"), ALBEDO_TEXTURE_UNIT);

    m_locAlbedoColor = glGetUniformLocation(w_program, "uAlbedoColor");
    m_locEmissive = glGetUniformLocation(w_program, "uEmissive");
    m_locRoughness = glGetUniformLocation(w_program, "uRoughness");
    m_locMetallic = glGetUniformLocation(w_program, "uMetallic");
    m_locOpacity = glGetUniformLocation(w_program, "uOpacity");
}

void Renderer::submitLights(std::vector<SceneLight>& lights, const float time) {
    if (lights.empty()) return;
    lights[0].light.pos = glm::vec3(3.0f * cos(time), 3.0f, 3.0f * sin(time));    // UPDATE
    for (const SceneLight& l : lights) this->submitLight(l.light);
}

void Renderer::submitLight(const Light& light) {
    if (w_lights.size() >= MAX_LIGHTS) return;     // de momento se ignoran las que sobran
    w_lights.push_back(light);
}
 
void Renderer::submit(MeshId id, MaterialId matId, const glm::mat4& model, const glm::vec4& color, int kind) {
    assert(id != INVALID_MESH);
    assert(matId != INVALID_MATERIAL_ID);
    if (id >= w_batches.size()) {
        w_batches.resize(id + 1);
    }
    auto& batches = w_batches[id];

    auto it = std::find_if(batches.begin(), batches.end(), [matId](const RenderBatch& batch) { return batch.material == matId; });

    if (it == batches.end()) {
        batches.push_back({ matId, {} });
        it = std::prev(batches.end());
    }

    it->instances.push_back({ model, color, (float)kind });
}

void Renderer::drawShadowGeometry(const MeshLibrary& lib) const {
    for (MeshId i = 0; i < w_batches.size(); ++i) {
        for (const RenderBatch& batch : w_batches[i]) {
            lib.get(i).drawInstances(batch.instances);
        }
    }
}

void Renderer::applyLights(GLuint program) const {
    glUniform1i(glGetUniformLocation(program, "uNumLights"), (GLint)w_lights.size());
    if (!w_lights.empty())
        glUniform4fv(glGetUniformLocation(program, "uLights[0]"),
            (GLsizei)(w_lights.size() * 2), (const GLfloat*)w_lights.data());
} 

void Renderer::drawLighting(const MeshLibrary& lib, const MaterialLibrary& matLib, const glm::mat4& viewProj,
    const glm::vec3& cameraPosition) const {
    glUseProgram(w_program);   // tras cada vista queda activo el programa de los personajes
    glUniformMatrix4fv(w_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    if (w_locCamPos >= 0) glUniform3fv(w_locCamPos, 1, glm::value_ptr(cameraPosition));   // los brillos dependen de desde donde mira cada vista
    applyLights(w_program);
    glActiveTexture(GL_TEXTURE0 + ALBEDO_TEXTURE_UNIT);

    auto drawBatch = [&](MeshId meshId, const RenderBatch& batch) {
        if (batch.instances.empty()) return;
        const Material& mat = matLib.get(batch.material);
        glUniform4fv(m_locAlbedoColor, 1, &mat.albedoColor[0]);
        glUniform3fv(m_locEmissive, 1, &mat.emissive[0]);
        glUniform1f(m_locRoughness, mat.roughness);
        glUniform1f(m_locMetallic, mat.metallic);
        glUniform1f(m_locOpacity, mat.opacity);
        glBindTexture(GL_TEXTURE_2D, mat.albedoTexture);
        lib.get(meshId).drawInstances(batch.instances);
        };

    // 1. Geometria opaca
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    for (MeshId i = 0; i < w_batches.size(); ++i)
        for (const RenderBatch& batch : w_batches[i])
            if (matLib.get(batch.material).opacity >= 1.0f) drawBatch(i, batch);

    // 2. Geometria transparente, despues: tiene que mezclarse con lo que ya esta pintado
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    for (MeshId i = 0; i < w_batches.size(); ++i)
        for (const RenderBatch& batch : w_batches[i])
            if (matLib.get(batch.material).opacity < 1.0f) drawBatch(i, batch);

    // Restaurar el estado de OpenGL
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::clearFrame() {
    for (std::vector<RenderBatch>& meshBatches : w_batches) meshBatches.clear();
    w_lights.clear();
}