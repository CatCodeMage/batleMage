#include "gfx/Renderer.h"

#include <algorithm>
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>

#include "gfx/MeshFactory.h"

namespace gfx {

namespace {

const char* kVertexSrc = R"glsl(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uViewProj;
uniform mat3 uNormalMat;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal = normalize(uNormalMat * aNormal);
    vUV = aUV;
    gl_Position = uViewProj * wp;
}
)glsl";

const char* kFragmentSrc = R"glsl(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
out vec4 FragColor;

uniform vec3 uColor;
uniform vec3 uEmissive;
uniform float uAlpha;
uniform int uUnlit;
uniform int uUseTex;
uniform sampler2D uTex;
uniform float uSpecular;
uniform float uShininess;

uniform vec3 uCamPos;
uniform vec3 uLightDir;      // hacia la luz
uniform vec3 uLightColor;
uniform vec3 uAmbient;

uniform int uNumPoint;
uniform vec3 uPointPos[8];
uniform vec3 uPointColor[8];

void main() {
    vec3 base = uColor;
    if (uUseTex != 0) base *= texture(uTex, vUV).rgb;
    if (uUnlit != 0) {
        FragColor = vec4(base, uAlpha);
        return;
    }

    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCamPos - vWorldPos);
    if (dot(N, V) < 0.0) N = -N;   // iluminacion a doble cara

    vec3 L = normalize(uLightDir);
    float diff = max(dot(N, L), 0.0);
    vec3 H = normalize(L + V);
    float spec = (diff > 0.0) ? uSpecular * pow(max(dot(N, H), 0.0), uShininess) : 0.0;

    vec3 lit = uAmbient + uLightColor * diff;
    vec3 specCol = uLightColor * spec;

    for (int i = 0; i < uNumPoint; ++i) {
        vec3 d = uPointPos[i] - vWorldPos;
        float dist = length(d);
        vec3 Lp = d / max(dist, 0.001);
        float att = 1.0 / (1.0 + 0.35 * dist + 0.08 * dist * dist);
        float dp = max(dot(N, Lp), 0.0);
        lit += uPointColor[i] * dp * att * 2.5;
        vec3 Hp = normalize(Lp + V);
        if (dp > 0.0) specCol += uPointColor[i] * uSpecular * pow(max(dot(N, Hp), 0.0), uShininess) * att;
    }

    vec3 color = base * lit + specCol + uEmissive;
    FragColor = vec4(color, uAlpha);
}
)glsl";

const glm::vec3 kX(1.0f, 0.0f, 0.0f);
const glm::vec3 kY(0.0f, 1.0f, 0.0f);
const glm::vec3 kZ(0.0f, 0.0f, 1.0f);

glm::vec3 teamColor(int i) {
    return i == 0 ? glm::vec3(0.25f, 0.45f, 1.0f) : glm::vec3(1.0f, 0.35f, 0.30f);
}

glm::mat4 basisMatrix(const glm::vec3& right, const glm::vec3& up, const glm::vec3& fwd) {
    return glm::mat4(glm::vec4(right, 0.0f), glm::vec4(up, 0.0f), glm::vec4(fwd, 0.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
}

// Matriz que proyecta cualquier punto sobre el plano y=h siguiendo la direccion de la luz (sombra plana).
glm::mat4 shadowMatrix(const glm::vec3& toLight, float h) {
    glm::mat4 s(1.0f);
    s[1] = glm::vec4(-toLight.x / toLight.y, 0.0f, -toLight.z / toLight.y, 0.0f);
    s[3] = glm::vec4(toLight.x / toLight.y * h, h, toLight.z / toLight.y * h, 1.0f);
    return s;
}

}  // namespace

Renderer::Renderer()
    : shader_(kVertexSrc, kFragmentSrc),
      cube_(makeCube()),
      sphere_(makeSphere()),
      plane_(makePlane(14.0f, 3.0f)),
      floorTex_(Texture::checker(256, 8, glm::vec3(0.55f, 0.52f, 0.60f), glm::vec3(0.38f, 0.36f, 0.45f))),
      lightDir_(glm::normalize(glm::vec3(0.45f, 1.0f, 0.35f))) {}

void Renderer::setLights(const sim::Battle& battle) const {
    glm::vec3 pos[8];
    glm::vec3 col[8];
    int n = 0;
    for (const sim::Projectile& p : battle.projectiles()) {
        if (n >= 8) break;
        pos[n] = p.pos;
        col[n] = teamColor(p.owner);
        ++n;
    }
    shader_.setInt("uNumPoint", n);
    shader_.setVec3Array("uPointPos", pos, n);
    shader_.setVec3Array("uPointColor", col, n);
}

void Renderer::draw(const Mesh& mesh, const glm::mat4& model, const Material& m) const {
    shader_.setMat4("uModel", model);
    shader_.setMat3("uNormalMat", glm::transpose(glm::inverse(glm::mat3(model))));
    shader_.setVec3("uColor", m.color);
    shader_.setVec3("uEmissive", m.emissive);
    shader_.setFloat("uAlpha", m.alpha);
    shader_.setFloat("uSpecular", m.specular);
    shader_.setFloat("uShininess", m.shininess);
    shader_.setInt("uUnlit", m.unlit ? 1 : 0);
    shader_.setInt("uUseTex", m.textured ? 1 : 0);
    mesh.draw();
}

void Renderer::addFighter(const sim::Battle& b, int i, std::vector<Part>& out) const {
    const sim::Fighter& f = b.fighter(i);
    const float t = b.time();
    const float yaw = std::atan2(f.facing.x, f.facing.z);

    glm::mat4 base = glm::translate(glm::mat4(1.0f), f.pos) * glm::rotate(glm::mat4(1.0f), yaw, kY);
    float castProg = 0.0f;

    if (f.alive()) {
        base = glm::translate(base, glm::vec3(0.0f, 0.04f * std::sin(t * 3.0f + static_cast<float>(i) * 1.7f), 0.0f));
        if (f.current == sim::Action::Dodge) {   // se inclina hacia el lado de la esquiva
            const float side = glm::dot(f.dodgeDir, glm::cross(kY, f.facing)) > 0.0f ? 1.0f : -1.0f;
            base = base * glm::rotate(glm::mat4(1.0f), -side * 0.45f * std::sin(3.14159f * b.actionProgress(i)), kZ);
        }
        if (f.current == sim::Action::CastMagic) {   // se inclina al lanzar
            castProg = std::min(1.0f, f.actionElapsed / b.config().rules.magicWindup);
            base = base * glm::rotate(glm::mat4(1.0f), 0.25f * castProg, kX);
        }
    } else {   // cae hacia atras
        const float fall = std::min(1.0f, f.deadTime * 2.5f) * 1.5f;
        base = base * glm::rotate(glm::mat4(1.0f), -fall, kX);
    }

    const glm::vec3 team = teamColor(i);
    const float flash = std::min(1.0f, f.hitTimer / 0.25f);   // destello blanco al recibir un golpe

    Material robe;
    robe.color = glm::mix(team, glm::vec3(1.0f), flash);
    robe.specular = 0.15f;
    Material skin;
    skin.color = glm::mix(glm::vec3(0.95f, 0.80f, 0.65f), glm::vec3(1.0f), flash);
    Material hat;
    hat.color = glm::mix(team * 0.55f, glm::vec3(1.0f), flash);
    Material wood;
    wood.color = glm::vec3(0.45f, 0.30f, 0.15f);
    wood.specular = 0.05f;

    out.push_back({&cube_, base * glm::translate(glm::mat4(1.0f), {0.0f, 0.9f, 0.0f}) * glm::scale(glm::mat4(1.0f), {0.75f, 1.15f, 0.5f}), robe, true});
    out.push_back({&sphere_, base * glm::translate(glm::mat4(1.0f), {0.0f, 1.78f, 0.0f}) * glm::scale(glm::mat4(1.0f), glm::vec3(0.28f)), skin, true});
    out.push_back({&cube_, base * glm::translate(glm::mat4(1.0f), {0.0f, 1.98f, 0.0f}) * glm::scale(glm::mat4(1.0f), {0.75f, 0.05f, 0.75f}), hat, true});
    out.push_back({&cube_, base * glm::translate(glm::mat4(1.0f), {0.0f, 2.20f, 0.0f}) * glm::scale(glm::mat4(1.0f), {0.32f, 0.40f, 0.32f}), hat, true});

    // Baston (se levanta al lanzar magia) y orbe brillante
    const glm::mat4 staff = base * glm::translate(glm::mat4(1.0f), {0.55f, 0.95f, 0.20f}) * glm::rotate(glm::mat4(1.0f), 0.15f + 0.9f * castProg, kX);
    out.push_back({&cube_, staff * glm::scale(glm::mat4(1.0f), {0.08f, 1.7f, 0.08f}), wood, true});

    Material orb;
    orb.color = team;
    orb.emissive = team * (0.6f + 1.4f * castProg);
    orb.specular = 0.8f;
    out.push_back({&sphere_, staff * glm::translate(glm::mat4(1.0f), {0.0f, 0.95f, 0.0f}) * glm::scale(glm::mat4(1.0f), glm::vec3(0.14f + 0.08f * castProg)), orb, false});
}

void Renderer::addProjectiles(const sim::Battle& b, std::vector<Part>& out) const {
    for (const sim::Projectile& p : b.projectiles()) {
        const glm::vec3 dir = glm::normalize(p.vel);
        glm::vec3 right = glm::cross(kY, dir);
        right = glm::length(right) > 1e-4f ? glm::normalize(right) : kX;
        const glm::vec3 up = glm::cross(dir, right);

        Material m;
        m.color = teamColor(p.owner);
        m.emissive = teamColor(p.owner) * 1.6f;
        m.specular = 0.8f;
        // El "palo" de energia: una barra alargada en la direccion del movimiento
        out.push_back({&cube_, glm::translate(glm::mat4(1.0f), p.pos) * basisMatrix(right, up, dir) * glm::scale(glm::mat4(1.0f), {0.10f, 0.10f, 0.90f}), m, true});
    }
}

void Renderer::addPillars(const sim::Battle& b, std::vector<Part>& out) const {
    const float r = b.config().rules.arenaRadius + 0.3f;
    Material stone;
    stone.color = glm::vec3(0.28f, 0.25f, 0.34f);
    stone.emissive = glm::vec3(0.10f, 0.03f, 0.18f);
    stone.specular = 0.1f;

    const int n = 24;
    for (int k = 0; k < n; ++k) {
        const float a = 6.2831853f * static_cast<float>(k) / static_cast<float>(n);
        out.push_back({&cube_, glm::translate(glm::mat4(1.0f), {r * std::cos(a), 0.6f, r * std::sin(a)}) * glm::scale(glm::mat4(1.0f), {0.3f, 1.2f, 0.3f}), stone, true});
    }
}

void Renderer::collectParts(const sim::Battle& b, std::vector<Part>& out) const {
    Material floor;
    floor.color = glm::vec3(1.0f);
    floor.textured = true;
    floor.specular = 0.05f;
    out.push_back({&plane_, glm::mat4(1.0f), floor, false});

    addPillars(b, out);
    for (int i = 0; i < 2; ++i) addFighter(b, i, out);
    addProjectiles(b, out);
}

void Renderer::drawShields(const sim::Battle& b) const {
    for (int i = 0; i < 2; ++i) {
        if (!b.isShielding(i)) continue;
        const float p = b.actionProgress(i);
        Material m;
        m.color = glm::vec3(0.40f, 0.80f, 1.0f);
        m.emissive = glm::vec3(0.15f, 0.40f, 0.70f);
        m.alpha = 0.42f * (1.0f - 0.5f * p);
        m.specular = 0.9f;
        draw(sphere_, glm::translate(glm::mat4(1.0f), b.shieldCenter(i)) * glm::scale(glm::mat4(1.0f), glm::vec3(b.config().rules.shieldRadius)), m);
    }
}

void Renderer::drawBars(const sim::Battle& b, const AutoCamera& cam) const {
    glm::vec3 fwd = glm::normalize(cam.target() - cam.position());
    glm::vec3 right = glm::cross(fwd, kY);
    right = glm::length(right) > 1e-4f ? glm::normalize(right) : kX;
    const glm::mat4 orient = basisMatrix(right, kY, glm::cross(right, kY));
    const glm::vec3 toCam = -fwd;

    const float w = 1.4f;
    for (int i = 0; i < 2; ++i) {
        const sim::Fighter& f = b.fighter(i);
        if (!f.alive()) continue;
        const sim::Rules& r = b.config().rules;
        const float hp = std::clamp(f.hp / r.maxHp, 0.0f, 1.0f);
        const float mp = std::clamp(f.mana / r.maxMana, 0.0f, 1.0f);
        const glm::vec3 anchor = f.pos + glm::vec3(0.0f, 2.85f, 0.0f);

        Material bg;
        bg.unlit = true;
        bg.color = glm::vec3(0.05f);
        draw(cube_, glm::translate(glm::mat4(1.0f), anchor) * orient * glm::scale(glm::mat4(1.0f), {w + 0.08f, 0.26f, 0.02f}), bg);

        Material hpM;
        hpM.unlit = true;
        hpM.color = glm::mix(glm::vec3(0.9f, 0.15f, 0.1f), glm::vec3(0.2f, 0.9f, 0.25f), hp);
        const float hw = std::max(0.001f, w * hp);
        draw(cube_, glm::translate(glm::mat4(1.0f), anchor + toCam * 0.03f - right * ((w - hw) * 0.5f) + kY * 0.06f) * orient * glm::scale(glm::mat4(1.0f), {hw, 0.10f, 0.02f}), hpM);

        Material mpM;
        mpM.unlit = true;
        mpM.color = glm::vec3(0.25f, 0.55f, 1.0f);
        const float mw = std::max(0.001f, w * mp);
        draw(cube_, glm::translate(glm::mat4(1.0f), anchor + toCam * 0.03f - right * ((w - mw) * 0.5f) - kY * 0.06f) * orient * glm::scale(glm::mat4(1.0f), {mw, 0.06f, 0.02f}), mpM);
    }
}

void Renderer::render(const sim::Battle& battle, const AutoCamera& camera, int width, int height) {
    glViewport(0, 0, width, height);
    glClearColor(0.04f, 0.05f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);

    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

    shader_.use();
    shader_.setMat4("uViewProj", camera.projection(aspect) * camera.view());
    shader_.setVec3("uCamPos", camera.position());
    shader_.setVec3("uLightDir", lightDir_);
    shader_.setVec3("uLightColor", glm::vec3(0.85f, 0.82f, 0.78f));
    shader_.setVec3("uAmbient", glm::vec3(0.22f, 0.24f, 0.32f));
    shader_.setInt("uTex", 0);
    setLights(battle);
    floorTex_.bind(0);

    std::vector<Part> parts;
    collectParts(battle, parts);

    // Pasada opaca
    for (const Part& p : parts) draw(*p.mesh, p.model, p.material);

    // Pasada de transparencias: sombras planas y escudos
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    const glm::mat4 shadow = shadowMatrix(lightDir_, 0.02f);
    Material shadowM;
    shadowM.unlit = true;
    shadowM.color = glm::vec3(0.0f);
    shadowM.alpha = 0.30f;
    for (const Part& p : parts) {
        if (p.castsShadow) draw(*p.mesh, shadow * p.model, shadowM);
    }
    drawShields(battle);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    drawBars(battle, camera);
}

}  // namespace gfx
