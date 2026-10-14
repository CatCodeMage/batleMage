#include "Renderer.h"
#include "ShadowMath.h"
#include "shaders.h"

Renderer::Renderer() {
    w_locViewProj = -1;
    w_locNumLights = -1;
    w_locLights = -1;
}

void Renderer::init(GLuint program) {
    destroy();
    w_program = program;
    w_locViewProj = glGetUniformLocation(program, "uViewProj");
    w_locNumLights = glGetUniformLocation(program, "uNumLights");
    w_locLights = glGetUniformLocation(program, "uLights[0]");
    w_locShadowLight = glGetUniformLocation(program, "uShadowLight");
    w_locShadowFar = glGetUniformLocation(program, "uShadowFar");
    w_locShadowTexel = glGetUniformLocation(program, "uShadowTexelSize");
    glUseProgram(program);
    glUniform1i(glGetUniformLocation(program, "uShadowMap"), 0);
    // Tambien antes del primer flush: nunca usar una textura aun no inicializada.
    glUniform1i(w_locShadowLight, -1);

    w_shadowProgram = createShaderProgram(shadowVertexSrc, shadowFragmentSrc);
    if (!w_shadowProgram || !w_shadowMap.init()) {
        std::cerr << "Sombras no disponibles; se mantiene la iluminacion normal.\n";
        w_shadowMap.destroy();
        glDeleteProgram(w_shadowProgram);
        w_shadowProgram = 0;
        return;
    }
    w_locLightViewProj = glGetUniformLocation(w_shadowProgram, "uLightViewProj");
    w_locLightPos = glGetUniformLocation(w_shadowProgram, "uLightPos");
    w_locFarPlane = glGetUniformLocation(w_shadowProgram, "uFarPlane");
}

void Renderer::destroy() {
    w_shadowMap.destroy();
    glDeleteProgram(w_shadowProgram);
    w_shadowProgram = w_program = 0;
    w_lights.clear();
    w_batches.clear();
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

void Renderer::flush(const MeshLibrary& lib, const glm::mat4& viewProj) {
    // Una sola transferencia de instancias por malla, no siete por frame.
    for (MeshId i = 0; i < w_batches.size(); ++i)
        if (!w_batches[i].empty()) lib.get(i).uploadInstances(w_batches[i]);

    int shadowIndex = -1;
    if (w_shadowsEnabled && shadowsReady() && w_shadowLightIndex >= 0 &&
        w_shadowLightIndex < (int)w_lights.size()) {
        const Light& light = w_lights[w_shadowLightIndex];
        if (ShadowMath::validLight(light.pos, light.radius) && light.intensity > 0.0f) {
            renderShadowMap(lib, light);
            shadowIndex = w_shadowLightIndex;
        }
    }

    glUseProgram(w_program);
    glUniformMatrix4fv(w_locViewProj, 1, GL_FALSE, glm::value_ptr(viewProj));
    glUniform1i(w_locShadowLight, shadowIndex);
    if (shadowIndex >= 0) {
        glUniform1f(w_locShadowFar, w_lights[shadowIndex].radius);
        glUniform1f(w_locShadowTexel, 1.0f / w_shadowMap.resolution());
    }

    // El renderer reserva la unidad 0 durante el dibujo y restaura su enlace.
    GLint previousActive = 0, previousCube = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActive);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_CUBE_MAP, &previousCube);
    glBindTexture(GL_TEXTURE_CUBE_MAP, w_shadowMap.texture());

    glUniform1i(w_locNumLights, (GLint)w_lights.size());
    if (!w_lights.empty())
        glUniform4fv(w_locLights, (GLsizei)(w_lights.size() * 2), (const GLfloat*)w_lights.data());

    drawBatches(lib);
    glBindTexture(GL_TEXTURE_CUBE_MAP, previousCube);
    glActiveTexture(previousActive);

    for (auto& batch : w_batches) batch.clear();
    w_lights.clear();
}

void Renderer::drawBatches(const MeshLibrary& lib) const {
    for (MeshId i = 0; i < w_batches.size(); ++i)
        if (!w_batches[i].empty())
            lib.get(i).drawUploadedInstances((GLsizei)w_batches[i].size());
}

void Renderer::renderShadowMap(const MeshLibrary& lib, const Light& light) {
    // La pasada de profundidad vuelve al mismo destino y tamano que tenia la escena.
    GLint previousFramebuffer = 0, viewport[4];
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousFramebuffer);
    glGetIntegerv(GL_VIEWPORT, viewport);
    glUseProgram(w_shadowProgram);
    glUniform3fv(w_locLightPos, 1, glm::value_ptr(light.pos));
    glUniform1f(w_locFarPlane, light.radius);
    const auto matrices = ShadowMath::faceMatrices(light.pos, light.radius);
    for (unsigned face = 0; face < matrices.size(); ++face) {
        w_shadowMap.beginFace(face);
        glUniformMatrix4fv(w_locLightViewProj, 1, GL_FALSE, glm::value_ptr(matrices[face]));
        drawBatches(lib);
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousFramebuffer);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}
