#include "Renderer.h"

constexpr GLint ALBEDO_TEXTURE_UNIT = 0;

Renderer::Renderer() {
    w_locViewProj = -1;
    w_locNumLights = -1;
    w_locLights = -1;
    w_locAlbedo = -1;
    w_locEmission = -1;
    w_locCamPos = -1;
}

void Renderer::init(GLuint program) {
    w_locViewProj = glGetUniformLocation(program, "uViewProj");
    w_locNumLights = glGetUniformLocation(program, "uNumLights");
    w_locLights = glGetUniformLocation(program, "uLights[0]");
    w_locAlbedo = glGetUniformLocation(program, "uAlbedo");
    w_locEmission = glGetUniformLocation(program, "uEmission");
    w_locCamPos = glGetUniformLocation(program, "uCamPos");
    glUniform1i(w_locAlbedo, ALBEDO_TEXTURE_UNIT);
}

void Renderer::submitLights(std::vector<SceneLight>& lights, const float time) {
    lights[0].light.pos = glm::vec3(3.0f * cos(time), 3.0f, 3.0f * sin(time));    // UPDATE
    for (const SceneLight& l : lights) this->submitLight(l.light);
}

void Renderer::submitLight(const Light& light) {
    if (w_lights.size() >= MAX_LIGHTS) return;     // de momento se ignoran las que sobran
    w_lights.push_back(light);
}
 
void Renderer::submit(MeshId id, GLuint albedoTexture, const glm::mat4& model, const glm::vec4& color,
    int kind, const glm::vec3& emission) {
    assert(id != INVALID_MESH);
    if (id >= w_batches.size()) {
        w_batches.resize(id + 1);
        w_batchMaterials.resize(id + 1);
    }
    w_batches[id].push_back({ model, color, (float)kind });
    w_batchMaterials[id] = { albedoTexture, emission };
}

void Renderer::drawShadowGeometry(const MeshLibrary& lib) const {
    for (MeshId i = 0; i < w_batches.size(); ++i) {
        if (!w_batches[i].empty() && w_batches[i][0].kind < 2.5f) {
            lib.get(i).drawInstances(w_batches[i]);
        }
    }
}

void Renderer::flushLigth() {
    glUniform1i(w_locNumLights, (GLint)w_lights.size());
    if (!w_lights.empty())
        glUniform4fv(w_locLights, (GLsizei)(w_lights.size() * 2), (const GLfloat*)w_lights.data());
} 

void Renderer::getActualViewProj(const glm::mat4& viewProj) {
    glUniformMatrix4fv(w_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
}

void Renderer::flush(const MeshLibrary& lib, const glm::mat4& viewProj) {
    getActualViewProj(viewProj);
    if (w_locCamPos >= 0)
        glUniform3fv(w_locCamPos, 1, glm::value_ptr(w_camPos));

    flushLigth();

    glActiveTexture(GL_TEXTURE0 + ALBEDO_TEXTURE_UNIT);
    // 1. Geometria opaca (escenario, personajes, proyectiles solidos, cristales y shards)
    for (MeshId i = 0; i < w_batches.size(); ++i) {
        if (!w_batches[i].empty() && w_batches[i][0].kind < 2.5f) {
            const BatchMaterial& material = w_batchMaterials[i];
            glBindTexture(GL_TEXTURE_2D, material.albedoTexture);
            glUniform3fv(w_locEmission, 1, glm::value_ptr(material.emission));
            lib.get(i).drawInstances(w_batches[i]);
            w_batches[i].clear();
        }
    }

    // 2. VFX transparentes (escarcha sobre superficie, niebla fria y sparkles)
    bool hasTransparent = false;
    for (MeshId i = 0; i < w_batches.size(); ++i) {
        if (!w_batches[i].empty()) {
            hasTransparent = true;
            break;
        }
    }
    if (hasTransparent) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);
        for (MeshId i = 0; i < w_batches.size(); ++i) {
            if (!w_batches[i].empty()) {
                const BatchMaterial& material = w_batchMaterials[i];
                glBindTexture(GL_TEXTURE_2D, material.albedoTexture);
                glUniform3fv(w_locEmission, 1, glm::value_ptr(material.emission));
                lib.get(i).drawInstances(w_batches[i]);
                w_batches[i].clear();
            }
        }
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    w_lights.clear();
}
