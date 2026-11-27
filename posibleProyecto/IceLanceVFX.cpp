// Adaptacion a OpenGL 3.3 C++ del VFX procedural de Ice Lance e Impact Shards de GLACILUX Lite.
// Basado en glacilux_meshes.gd, glacilux_projectile_3d.gd, glacilux_impact_3d.gd y glacilux_noise.gdshaderinc
// Copyright (c) 2026 gennai (https://gennai.itch.io) — MIT License.

#include "IceLanceVFX.h"
#include "tiny_obj_loader.h"
#include "CombatSimulation.h"
#include <algorithm>
#include <cmath>

namespace {

constexpr float GX_PI  = 3.14159265358979323846f;
constexpr float GX_TAU = 6.28318530717958647692f;

// Kinds de shader en shaders.cpp
constexpr int KIND_ICE_CRYSTAL = 2;
constexpr int KIND_COLD_MIST   = 3;
constexpr int KIND_SPARKLE     = 4;
constexpr int KIND_FROST_PATCH = 5;
constexpr int KIND_FIRE_ORB    = 6;
constexpr int KIND_FIRE_FLAME  = 7;
constexpr int KIND_FIRE_FLASH  = 8;
constexpr int KIND_FIRE_SMOKE     = 9;
constexpr int KIND_FIRE_DECAL     = 10;
constexpr int KIND_LIGHTNING_BOLT = 11;
constexpr int KIND_WIND_ORB       = 12;
constexpr int KIND_WIND_STREAK    = 13;
constexpr int KIND_WIND_DECAL     = 14;
constexpr int KIND_WATER_ORB      = 15;
constexpr int KIND_WATER_DROP     = 16;
constexpr int KIND_WATER_MIST     = 17;

// Paleta original de GlaciluxEffect3D
const glm::vec4 ICE_TINT(0.75f, 0.90f, 1.0f, 1.0f);
const glm::vec3 ICE_EMISSION(0.33f, 0.51f, 0.60f);
const glm::vec3 ICE_LIGHT_COLOR(0.45f, 0.72f, 1.0f);
const glm::vec3 FIRE_LIGHT_COLOR(1.0f, 0.50f, 0.15f);
const glm::vec3 FIRE_CAST_LIGHT_COLOR(1.0f, 0.55f, 0.20f);
const glm::vec3 LIGHTNING_LIGHT_COLOR(0.60f, 0.75f, 1.0f);
const glm::vec3 EARTH_LIGHT_COLOR(1.0f, 0.69f, 0.38f);
const glm::vec3 WIND_LIGHT_COLOR(0.75f, 0.88f, 1.0f);

float gx_fract(float x) {
    return x - std::floor(x);
}

glm::vec3 gx_fract(const glm::vec3& v) {
    return glm::vec3(gx_fract(v.x), gx_fract(v.y), gx_fract(v.z));
}

float gx_clamp(float v, float lo, float hi) {
    return (std::max)(lo, (std::min)(v, hi));
}

float gx_smoothstep(float edge0, float edge1, float x) {
    const float t = gx_clamp((x - edge0) / (std::max)(edge1 - edge0, 0.0001f), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}

// Funciones deterministas de glacilux_noise.gdshaderinc
float gx_hash11(float p) {
    p = gx_fract(p * 0.1031f);
    p *= p + 33.33f;
    p *= p + p;
    return gx_fract(p);
}

glm::vec3 gx_hash33(glm::vec3 p) {
    glm::vec3 p3 = gx_fract(p * glm::vec3(0.1031f, 0.1030f, 0.0973f));
    const float d = glm::dot(p3, glm::vec3(p3.y, p3.x, p3.z) + glm::vec3(33.33f));
    p3 += glm::vec3(d);
    return gx_fract((glm::vec3(p3.x, p3.x, p3.y) + glm::vec3(p3.y, p3.x, p3.x)) * glm::vec3(p3.z, p3.y, p3.x));
}

glm::vec3 gx_unit_from_index(float idx, float salt) {
    const glm::vec3 h = gx_hash33(glm::vec3(idx, salt, idx * 0.37f + salt));
    const glm::vec3 v = h * 2.0f - glm::vec3(1.0f);
    const float len = glm::length(v);
    return len > 0.0001f ? (v / len) : glm::vec3(0.0f, 1.0f, 0.0f);
}

float gx_overshoot(float t, float amount) {
    t = gx_clamp(t, 0.0f, 1.0f);
    const float inv = 1.0f - t;
    const float s = 1.0f - inv * inv * inv;
    return s + amount * std::sin(t * GX_PI) * (1.0f - t);
}

// Construye una base ortonormal cuyo eje +Y es la normal de la superficie `n`
glm::mat3 gx_basis_from_normal(const glm::vec3& normal) {
    const glm::vec3 n = glm::length(normal) > 0.0001f ? glm::normalize(normal) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 up = std::abs(n.y) < 0.95f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
    const glm::vec3 t = glm::normalize(glm::cross(up, n));
    const glm::vec3 b = glm::cross(n, t);
    return glm::mat3(t, n, b);
}

glm::mat3 gx_rot_axis(const glm::vec3& axis, float ang) {
    const float c = std::cos(ang);
    const float s = std::sin(ang);
    const float k = 1.0f - c;
    const glm::vec3 a = glm::length(axis) > 0.0001f ? glm::normalize(axis) : glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::mat3(
        glm::vec3(c + a.x * a.x * k, a.x * a.y * k + a.z * s, a.x * a.z * k - a.y * s),
        glm::vec3(a.x * a.y * k - a.z * s, c + a.y * a.y * k, a.y * a.z * k + a.x * s),
        glm::vec3(a.x * a.z * k + a.y * s, a.y * a.z * k - a.x * s, c + a.z * a.z * k)
    );
}

// Constructor de geometria procedural con normales planas por cara
struct MeshBuilder {
    std::vector<float> verts;
    std::vector<unsigned> idx;

    void addVertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& uv) {
        verts.insert(verts.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y });
    }

    void addTriangle(
        const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2,
        float crackAmount, float tip0, float tip1, float tip2)
    {
        glm::vec3 fn = glm::cross(p1 - p0, p2 - p0);
        const float len = glm::length(fn);
        fn = len > 1e-6f ? (fn / len) : glm::vec3(0.0f, 1.0f, 0.0f);
        const unsigned base = static_cast<unsigned>(verts.size() / FLOATS_PER_VERTEX);
        addVertex(p0, fn, glm::vec2(crackAmount, tip0));
        addVertex(p1, fn, glm::vec2(crackAmount, tip1));
        addVertex(p2, fn, glm::vec2(crackAmount, tip2));
        idx.insert(idx.end(), { base, base + 1, base + 2 });
    }

    void addQuad(
        const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3,
        float crackAmount, float tip0, float tip1, float tip2, float tip3)
    {
        glm::vec3 fn = glm::cross(p1 - p0, p2 - p0);
        const float len = glm::length(fn);
        fn = len > 1e-6f ? (fn / len) : glm::vec3(0.0f, 1.0f, 0.0f);
        const unsigned base = static_cast<unsigned>(verts.size() / FLOATS_PER_VERTEX);
        addVertex(p0, fn, glm::vec2(crackAmount, tip0));
        addVertex(p1, fn, glm::vec2(crackAmount, tip1));
        addVertex(p2, fn, glm::vec2(crackAmount, tip2));
        addVertex(p3, fn, glm::vec2(crackAmount, tip3));
        idx.insert(idx.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
    }
};

// Construye la lanza de cristal de 6 caras (orientada hacia +Z) con punta alargada, cola facetada y aletas laterales
MeshId buildIceLanceBodyMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    constexpr int SIDES = 6;
    constexpr float CRACK = 0.6f;

    // Cuerpo principal (basado en body_length = 1.68, body_radius = 0.15225, escalado a la arena de BattleMage)
    const glm::vec3 tailTip(0.0f, 0.0f, -1.05f);
    const glm::vec3 headTip(0.0f, 0.0f,  1.45f);
    std::array<glm::vec3, SIDES> rearRing;
    std::array<glm::vec3, SIDES> midRing;
    std::array<glm::vec3, SIDES> frontRing;

    for (int k = 0; k < SIDES; ++k) {
        const float a = GX_TAU * static_cast<float>(k) / static_cast<float>(SIDES);
        const float ca = std::cos(a);
        const float sa = std::sin(a);
        rearRing[k]  = glm::vec3(ca * 0.20f, sa * 0.20f, -0.45f);
        midRing[k]   = glm::vec3(ca * 0.27f, sa * 0.27f,  0.05f);
        frontRing[k] = glm::vec3(ca * 0.19f, sa * 0.19f,  0.68f);
    }

    for (int k = 0; k < SIDES; ++k) {
        const int k2 = (k + 1) % SIDES;
        // Cono trasero (cola del cristal)
        b.addTriangle(tailTip, rearRing[k2], rearRing[k], CRACK, 0.85f, 0.20f, 0.20f);
        // Tramo trasero-medio
        b.addQuad(rearRing[k], rearRing[k2], midRing[k2], midRing[k], CRACK, 0.20f, 0.20f, 0.40f, 0.40f);
        // Tramo medio-delantero
        b.addQuad(midRing[k], midRing[k2], frontRing[k2], frontRing[k], CRACK, 0.40f, 0.40f, 0.68f, 0.68f);
        // Punta delantera de la lanza
        b.addTriangle(frontRing[k], frontRing[k2], headTip, CRACK, 0.68f, 0.68f, 1.0f);
    }

    // 4 puas laterales de cristal adheridas al collar de la lanza para una silueta mas agresiva
    for (int f = 0; f < 4; ++f) {
        const float a = GX_TAU * static_cast<float>(f) / 4.0f + 0.35f;
        const glm::vec3 dir(std::cos(a), std::sin(a), 0.0f);
        const glm::vec3 side(-dir.y, dir.x, 0.0f);
        const glm::vec3 b0 = dir * 0.14f + glm::vec3(0.0f, 0.0f, -0.42f);
        const glm::vec3 b1 = dir * 0.18f + side * 0.09f + glm::vec3(0.0f, 0.0f, -0.12f);
        const glm::vec3 b2 = dir * 0.18f - side * 0.09f + glm::vec3(0.0f, 0.0f, -0.12f);
        const glm::vec3 barbTip = dir * 0.42f + glm::vec3(0.0f, 0.0f, -0.58f);
        const glm::vec3 fwdTip  = dir * 0.36f + glm::vec3(0.0f, 0.0f,  0.42f);

        b.addTriangle(b0, b1, barbTip, CRACK, 0.3f, 0.4f, 0.95f);
        b.addTriangle(b2, b0, barbTip, CRACK, 0.4f, 0.3f, 0.95f);
        b.addTriangle(b1, b2, fwdTip,  CRACK, 0.4f, 0.4f, 0.92f);
        b.addTriangle(b2, b1, barbTip, CRACK, 0.4f, 0.4f, 0.95f);
    }

    return meshes.add("glacilux_ice_lance_body", b.verts, b.idx);
}

// Prisma apuntado de 5 caras para la corona de impacto (GlaciluxMeshes.surface_prisms)
MeshId buildCrownSpikeMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    constexpr int SIDES = 5;
    constexpr float CRACK = 0.4f;
    std::array<glm::vec3, SIDES> baseRing;
    std::array<glm::vec3, SIDES> midRing;

    for (int k = 0; k < SIDES; ++k) {
        const float a = GX_TAU * static_cast<float>(k) / static_cast<float>(SIDES);
        baseRing[k] = glm::vec3(std::cos(a), 0.0f, std::sin(a));
        midRing[k]  = glm::vec3(std::cos(a) * 0.74f, 0.58f, std::sin(a) * 0.74f);
    }
    const glm::vec3 tip(0.15f, 1.0f, 0.05f);

    for (int k = 0; k < SIDES; ++k) {
        const int k2 = (k + 1) % SIDES;
        b.addQuad(baseRing[k], midRing[k], midRing[k2], baseRing[k2], CRACK, 0.0f, 0.58f, 0.58f, 0.0f);
        b.addTriangle(midRing[k], tip, midRing[k2], CRACK, 0.58f, 1.0f, 0.58f);
    }
    return meshes.add("glacilux_crown_spike", b.verts, b.idx);
}

// Fragmento irregular de 6 caras (GlaciluxMeshes._emit_box con jitter)
MeshId buildShardBoxMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    constexpr float CRACK = 0.35f;
    const std::array<glm::vec3, 8> c = {
        glm::vec3(-0.56f, -0.44f, -0.48f),
        glm::vec3( 0.52f, -0.52f, -0.42f),
        glm::vec3( 0.46f,  0.55f, -0.54f),
        glm::vec3(-0.48f,  0.46f, -0.44f),
        glm::vec3(-0.45f, -0.53f,  0.54f),
        glm::vec3( 0.58f, -0.46f,  0.48f),
        glm::vec3( 0.51f,  0.49f,  0.52f),
        glm::vec3(-0.54f,  0.52f,  0.45f)
    };
    const int faces[6][4] = {
        {0, 3, 2, 1},
        {4, 5, 6, 7},
        {0, 1, 5, 4},
        {3, 7, 6, 2},
        {0, 4, 7, 3},
        {1, 2, 6, 5}
    };
    for (int f = 0; f < 6; ++f) {
        b.addQuad(
            c[faces[f][0]], c[faces[f][1]], c[faces[f][2]], c[faces[f][3]],
            CRACK, 0.35f, 0.75f, 0.85f, 0.45f
        );
    }
    return meshes.add("glacilux_shard_box", b.verts, b.idx);
}

// Fragmento en forma de cuna triangular (GlaciluxMeshes._emit_wedge)
MeshId buildShardWedgeMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    constexpr float CRACK = 0.35f;
    const std::array<glm::vec3, 3> top = {
        glm::vec3( 0.55f,  0.58f,  0.08f),
        glm::vec3(-0.32f,  0.52f,  0.48f),
        glm::vec3(-0.28f,  0.62f, -0.52f)
    };
    const std::array<glm::vec3, 3> bot = {
        glm::vec3( 0.62f, -0.56f,  0.05f),
        glm::vec3(-0.38f, -0.60f,  0.54f),
        glm::vec3(-0.34f, -0.52f, -0.58f)
    };
    b.addTriangle(top[0], top[1], top[2], CRACK, 0.85f, 0.85f, 0.85f);
    b.addTriangle(bot[0], bot[2], bot[1], CRACK, 0.25f, 0.25f, 0.25f);
    for (int k = 0; k < 3; ++k) {
        const int k2 = (k + 1) % 3;
        b.addQuad(bot[k], bot[k2], top[k2], top[k], CRACK, 0.25f, 0.25f, 0.85f, 0.85f);
    }
    return meshes.add("glacilux_shard_wedge", b.verts, b.idx);
}

// Quad plano en XZ (y = 0) para el parche de escarcha sobre superficies
MeshId buildFrostQuadMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    const glm::vec3 n(0.0f, 1.0f, 0.0f);
    b.addVertex(glm::vec3(-1.0f, 0.0f, -1.0f), n, glm::vec2(0.0f, 0.0f));
    b.addVertex(glm::vec3( 1.0f, 0.0f, -1.0f), n, glm::vec2(1.0f, 0.0f));
    b.addVertex(glm::vec3( 1.0f, 0.0f,  1.0f), n, glm::vec2(1.0f, 1.0f));
    b.addVertex(glm::vec3(-1.0f, 0.0f,  1.0f), n, glm::vec2(0.0f, 1.0f));
    b.idx = { 0, 2, 1, 0, 3, 2, 0, 1, 2, 0, 2, 3 };
    return meshes.add("glacilux_frost_quad", b.verts, b.idx);
}

// Quad camera-facing en XY [-0.5, 0.5] para niebla fria y sparkles
MeshId buildBillboardQuadMesh(MeshLibrary& meshes, const std::string& name) {
    MeshBuilder b;
    const glm::vec3 n(0.0f, 0.0f, 1.0f);
    b.addVertex(glm::vec3(-0.5f, -0.5f, 0.0f), n, glm::vec2(0.0f, 0.0f));
    b.addVertex(glm::vec3( 0.5f, -0.5f, 0.0f), n, glm::vec2(1.0f, 0.0f));
    b.addVertex(glm::vec3( 0.5f,  0.5f, 0.0f), n, glm::vec2(1.0f, 1.0f));
    b.addVertex(glm::vec3(-0.5f,  0.5f, 0.0f), n, glm::vec2(0.0f, 1.0f));
    b.idx = { 0, 1, 2, 0, 2, 3 };
    return meshes.add(name, b.verts, b.idx);
}

// Esfera UV para el nucleo Core del proyectil de fuego (sp_orb.gdshader)
MeshId buildFireOrbMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    constexpr int RINGS = 12;
    constexpr int SEGMENTS = 20;
    for (int r = 0; r <= RINGS; ++r) {
        const float v = static_cast<float>(r) / static_cast<float>(RINGS);
        const float phi = v * GX_PI;
        const float sp = std::sin(phi);
        const float cp = std::cos(phi);
        for (int s = 0; s <= SEGMENTS; ++s) {
            const float u = static_cast<float>(s) / static_cast<float>(SEGMENTS);
            const float theta = u * GX_TAU;
            const glm::vec3 n(sp * std::cos(theta), cp, sp * std::sin(theta));
            b.addVertex(n, n, glm::vec2(u, v));
        }
    }
    const int stride = SEGMENTS + 1;
    for (int r = 0; r < RINGS; ++r) {
        for (int s = 0; s < SEGMENTS; ++s) {
            const unsigned i0 = static_cast<unsigned>(r * stride + s);
            const unsigned i1 = i0 + 1;
            const unsigned i2 = static_cast<unsigned>((r + 1) * stride + s);
            const unsigned i3 = i2 + 1;
            b.idx.insert(b.idx.end(), { i0, i2, i1, i1, i2, i3 });
        }
    }
    return meshes.add("reeboo_fire_orb", b.verts, b.idx);
}

// Geometria low-poly exacta de rock.obj (Lowpoly Nature & Earth VFX, Copyright (c) 2026 Laonion)
MeshId buildEarthRockMesh(MeshLibrary& meshes) {
    MeshBuilder b;
    const std::array<glm::vec3, 12> v = {
        glm::vec3(-0.656171f,  0.764429f,  0.000000f),
        glm::vec3(-0.839768f,  0.000000f,  0.467104f),
        glm::vec3( 0.000000f,  0.352345f,  0.712632f),
        glm::vec3( 0.529066f,  0.616354f,  0.000000f),
        glm::vec3( 0.000000f,  0.309535f, -0.626047f),
        glm::vec3(-1.042252f,  0.000000f, -0.579732f),
        glm::vec3( 0.871583f,  0.000000f,  0.484801f),
        glm::vec3( 0.000000f, -0.401283f,  0.811612f),
        glm::vec3(-0.665942f, -0.775812f,  0.000000f),
        glm::vec3( 0.000000f, -0.442522f, -0.895019f),
        glm::vec3( 1.027685f,  0.000000f, -0.571630f),
        glm::vec3( 0.402325f, -0.468703f,  0.000000f)
    };
    const int faces[20][3] = {
        {0, 1, 2}, {0, 2, 3}, {0, 3, 4}, {0, 4, 5}, {0, 5, 1},
        {3, 2, 6}, {2, 1, 7}, {1, 5, 8}, {5, 4, 9}, {4, 3, 10},
        {11, 6, 7}, {11, 7, 8}, {11, 8, 9}, {11, 9, 10}, {11, 10, 6},
        {7, 6, 2}, {8, 7, 1}, {9, 8, 5}, {10, 9, 4}, {6, 10, 3}
    };
    for (int f = 0; f < 20; ++f) {
        b.addTriangle(v[faces[f][0]], v[faces[f][1]], v[faces[f][2]], 0.0f, 0.5f, 0.5f, 0.5f);
    }
    return meshes.add("lowpoly_earth_rock", b.verts, b.idx);
}

glm::mat4 makeBillboardMatrix(
    const glm::vec3& pos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    float size)
{
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(camRight * size, 0.0f);
    m[1] = glm::vec4(camUp * size, 0.0f);
    m[2] = glm::vec4(camForward * size, 0.0f);
    m[3] = glm::vec4(pos, 1.0f);
    return m;
}

// Billboard rotado en el plano de camara (facing == 0 en sp_particle.gdshaderinc)
glm::mat4 makeRotatedBillboardMatrix(
    const glm::vec3& pos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    float size,
    float angleRad)
{
    const float c = std::cos(angleRad);
    const float s = std::sin(angleRad);
    const glm::vec3 r = (camRight * c + camUp * s) * size;
    const glm::vec3 u = (-camRight * s + camUp * c) * size;
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(r, 0.0f);
    m[1] = glm::vec4(u, 0.0f);
    m[2] = glm::vec4(camForward * size, 0.0f);
    m[3] = glm::vec4(pos, 1.0f);
    return m;
}

// Billboard estirado a lo largo de un eje 3D (facing == 1 en sp_particle.gdshaderinc)
glm::mat4 makeAlignedBillboardMatrix(
    const glm::vec3& pos,
    const glm::vec3& axisDir,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    float width,
    float length)
{
    const float axLen = glm::length(axisDir);
    const glm::vec3 axis = axLen > 0.0001f ? (axisDir / axLen) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 toCam = glm::length(camPos - pos) > 0.0001f
        ? glm::normalize(camPos - pos)
        : glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 side = glm::cross(axis, toCam);
    const float sideLen = glm::length(side);
    side = sideLen > 0.0001f ? (side / sideLen) : camRight;
    const glm::vec3 fwd = glm::cross(side, axis);
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(side * width, 0.0f);
    m[1] = glm::vec4(axis * length, 0.0f);
    m[2] = glm::vec4(fwd * width, 0.0f);
    m[3] = glm::vec4(pos, 1.0f);
    return m;
}

// Reconstruye el vector de direccion 3D a partir de (pitch = rot.x, yaw = rot.y) grabados en el replay
glm::vec3 directionFromPitchYaw(const glm::vec3& rot) {
    const float cp = std::cos(rot.x);
    const glm::vec3 d(std::sin(rot.y) * cp, -std::sin(rot.x), std::cos(rot.y) * cp);
    return glm::length(d) > 0.0001f ? glm::normalize(d) : glm::vec3(0.0f, 0.0f, 1.0f);
}

// Desplazamiento con amortiguamiento exponencial (damping de ParticleProcessMaterial en Godot)
float dampedDistance(float speed, float damping, float age) {
    if (damping <= 0.001f) return speed * age;
    return speed * (1.0f - std::exp(-damping * age)) / damping;
}

// Rampa de color de llamas de REEBOO (amarillo-blanco -> naranja intenso -> rojo brasa)
glm::vec3 evalFireFlameRamp(float u, float seedFrac) {
    u = gx_clamp(u, 0.0f, 1.0f);
    glm::vec3 c;
    if (u < 0.30f) {
        c = glm::mix(glm::vec3(1.0f, 0.94f, 0.72f), glm::vec3(1.0f, 0.62f, 0.18f), u / 0.30f);
    } else if (u < 0.70f) {
        c = glm::mix(glm::vec3(1.0f, 0.62f, 0.18f), glm::vec3(0.95f, 0.26f, 0.04f), (u - 0.30f) / 0.40f);
    } else {
        c = glm::mix(glm::vec3(0.95f, 0.26f, 0.04f), glm::vec3(0.45f, 0.05f, 0.01f), (u - 0.70f) / 0.30f);
    }
    // Codifica una pequena variacion de semilla en B sin alterar el tono visual
    c.b = gx_clamp(c.b + seedFrac * 0.04f, 0.0f, 1.0f);
    return c;
}

// Direccion determinista dentro de un cono alrededor de `axis` (GlaciluxSamples.in_cone)
glm::vec3 sampleDirectionInCone(int index, const glm::vec3& axis, float degrees, float seedSalt) {
    const glm::mat3 basis = gx_basis_from_normal(axis);
    const float h1 = gx_hash11(static_cast<float>(index) * 1.37f + seedSalt);
    const float h2 = gx_hash11(static_cast<float>(index) * 2.71f + seedSalt + 11.0f);
    const float cosMin = std::cos(glm::radians(gx_clamp(degrees, 1.0f, 360.0f) * 0.5f));
    const float cz = 1.0f + (cosMin - 1.0f) * h1;
    const float sz = std::sqrt((std::max)(1.0f - cz * cz, 0.0f));
    const float a  = h2 * GX_TAU;
    const glm::vec3 localDir(std::cos(a) * sz, cz, std::sin(a) * sz);
    return glm::normalize(basis * localDir);
}

// Matriz de una malla con su eje local +Y alineado con `axis` (esfera de aire con las corrientes rodeando el vuelo)
glm::mat4 makeAxisAlignedMatrix(const glm::vec3& pos, const glm::vec3& axis, float scale) {
    const glm::mat3 b = gx_basis_from_normal(axis);
    glm::mat4 m(1.0f);
    m[0] = glm::vec4(b[0] * scale, 0.0f);
    m[1] = glm::vec4(b[1] * scale, 0.0f);
    m[2] = glm::vec4(b[2] * scale, 0.0f);
    m[3] = glm::vec4(pos, 1.0f);
    return m;
}

} // namespace

void IceLanceVFX::init(MeshLibrary& meshes, const TextureLibrary& textures, MaterialLibrary& matLib) {
    m_whiteTexture    = textures.white();

    Material whiteMat;
    whiteMat.name = "vfx_white";
    whiteMat.albedoTexture = m_whiteTexture;
    whiteMat.albedoColor = glm::vec4(1.0f);
    whiteMat.emissive = glm::vec3(0.0f);
    whiteMat.roughness = 1.0f;
    whiteMat.metallic = 0.0f;
    whiteMat.opacity = 1.0f;

    m_whiteMaterial = matLib.add(whiteMat, "vfx/white");

    // Orden de creacion: primero opacos (kind=2), luego transparentes en orden de prioridad
    m_lanceBodyMesh   = buildIceLanceBodyMesh(meshes);
    m_crownSpikeMesh  = buildCrownSpikeMesh(meshes);
    m_shardBoxMesh    = buildShardBoxMesh(meshes);
    m_shardWedgeMesh  = buildShardWedgeMesh(meshes);
    m_frostQuadMesh   = buildFrostQuadMesh(meshes);
    m_mistQuadMesh    = buildBillboardQuadMesh(meshes, "glacilux_mist_quad");
    m_sparkleQuadMesh = buildBillboardQuadMesh(meshes, "glacilux_sparkle_quad");

    // Meshes compartidos para Fire VFX (REEBOO Stylized Projectiles) en orden de renderizado:
    // Decals de superficie -> Humo -> Nucleo Orb 3D -> Llamas/Trail -> Flash/Brasas
    m_fireDecalMesh   = buildFrostQuadMesh(meshes);
    m_fireSmokeMesh   = buildBillboardQuadMesh(meshes, "reeboo_fire_smoke_quad");
    m_fireOrbMesh     = buildFireOrbMesh(meshes);
    m_fireFlameMesh   = buildBillboardQuadMesh(meshes, "reeboo_fire_flame_quad");
    m_fireFlashMesh   = buildBillboardQuadMesh(meshes, "reeboo_fire_flash_quad");
    m_earthRockMesh   = buildEarthRockMesh(meshes);
    
    m_daggerTexture = const_cast<TextureLibrary&>(textures).load("assets/dagger/throwing-knife_body_basecolor.jpg");

    if (m_daggerTexture == 0) {
        m_daggerTexture = m_whiteTexture;
    }
    
    Material daggerMat;

    daggerMat.name = "vfx_dagger";
    daggerMat.albedoTexture = m_daggerTexture;
    daggerMat.albedoColor = glm::vec4(1.0f);
    daggerMat.emissive = glm::vec3(0.0f);
    daggerMat.roughness = 0.8f;
    daggerMat.metallic = 0.0f;
    daggerMat.opacity = 1.0f;

    m_daggerMaterial = matLib.add(daggerMat, "vfx/dagger");

    // Cargar throwing-knife.obj una sola vez con tinyobjloader
    {
        tinyobj::ObjReader reader;
        tinyobj::ObjReaderConfig cfg;
        cfg.triangulate = true;
        if (reader.ParseFromFile("assets/dagger/throwing-knife.obj", cfg)) {
            const auto& attrib = reader.GetAttrib();
            std::vector<float> verts;
            std::vector<uint32_t> indices;
            for (const auto& shape : reader.GetShapes()) {
                for (const auto& idx : shape.mesh.indices) {
                    float px = 0.0f, py = 0.0f, pz = 0.0f;
                    float nx = 0.0f, ny = 1.0f, nz = 0.0f;
                    float u = 0.0f, v = 0.0f;
                    if (idx.vertex_index >= 0) {
                        // Rotar -90 deg en Y para alinear eje largo (+X) con +Z forward
                        const float ox = attrib.vertices[3 * idx.vertex_index + 0];
                        const float oy = attrib.vertices[3 * idx.vertex_index + 1];
                        const float oz = attrib.vertices[3 * idx.vertex_index + 2];
                        px = -oz; py = oy; pz = ox;
                    }
                    if (idx.normal_index >= 0) {
                        const float onx = attrib.normals[3 * idx.normal_index + 0];
                        const float ony = attrib.normals[3 * idx.normal_index + 1];
                        const float onz = attrib.normals[3 * idx.normal_index + 2];
                        nx = -onz; ny = ony; nz = onx;
                    }
                    if (idx.texcoord_index >= 0) {
                        u = attrib.texcoords[2 * idx.texcoord_index + 0];
                        v = 1.0f - attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    indices.push_back(static_cast<uint32_t>(verts.size() / 8));
                    verts.push_back(px); verts.push_back(py); verts.push_back(pz);
                    verts.push_back(nx); verts.push_back(ny); verts.push_back(nz);
                    verts.push_back(u);  verts.push_back(v);
                }
            }
            if (!verts.empty()) {
                m_daggerMesh = meshes.add("dagger_barrage_knife", verts, indices);
            }
        }
        if (m_daggerMesh == INVALID_MESH) {
            m_daggerMesh = m_earthRockMesh;
        }
    }
}

void IceLanceVFX::submitFrame(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    const std::vector<ImpactVFX>& impacts,
    int currentFrame,
    const Camera& camera,
    double timeSeconds) const
{
    if (m_lanceBodyMesh == INVALID_MESH || combatFrames.empty())
        return;

    const int clampedFrame = std::clamp(currentFrame, 0, static_cast<int>(combatFrames.size() - 1));
    const glm::mat4 view = camera.get_view();
    const glm::mat3 viewRot(view);
    const glm::mat3 invViewRot = glm::transpose(viewRot);
    const glm::vec3 camRight   = glm::normalize(invViewRot[0]);
    const glm::vec3 camUp      = glm::normalize(invViewRot[1]);
    const glm::vec3 camForward = glm::normalize(invViewRot[2]);
    const glm::vec3 camPos     = -(invViewRot * glm::vec3(view[3]));

    renderer.setCameraPos(camPos);

    submitFlightAndTrails(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitFireCastAndFlight(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitLightningCastAndFlight(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitEarthCastAndFlight(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitWindCastAndFlight(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitWaterCastAndFlight(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitDaggerCastAndFlight(renderer, combatFrames, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
    submitImpacts(renderer, impacts, clampedFrame, camPos, camRight, camUp, camForward, timeSeconds);
}

void IceLanceVFX::submitFlightAndTrails(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    // Configuración de projectile_ice_lance.tscn adaptada a la escala de BattleMage:
    // trail = 36 elementos, trail_span = 0.6 s (36 ticks a 60 Hz)
    constexpr int   TRAIL_TICKS     = 36;
    constexpr float TRAIL_SPAN      = 0.60f;
    constexpr float TRAIL_SIZE      = 0.42f;
    constexpr float TRAIL_SPREAD    = 0.48f;
    constexpr float TRAIL_SINK      = 0.65f;
    constexpr float SPARKLE_TWINKLE = 11.0f;
    constexpr float LANCE_SPIN_RAD  = 8.5f;

    // 1. Proyectiles Ice Lance activos en el frame actual
    for (const Entidad& e : combatFrames[currentFrame]) {
        if (!e.isIceLance)
            continue;

        const float age = static_cast<float>(e.animationAge >= 0.0 ? e.animationAge : timeSeconds);
        // Aparicion con overshoot rapido como en glacilux_projectile_3d.gd (rise_time = 0.07, overshoot = 0.4)
        const float grow = gx_overshoot(gx_clamp(age / 0.07f, 0.0f, 1.0f), 0.40f);

        Transform bodyTransform = e.transform;
        bodyTransform.scale = glm::vec3(1.10f, 1.10f, 1.15f) * grow;
        bodyTransform.rot.z = std::fmod(age * LANCE_SPIN_RAD, GX_TAU);

        const float seedFrac = gx_hash11(static_cast<float>(e.poolIndex >= 0 ? e.poolIndex * 19 + 7 : 17));
        renderer.submit(
            m_lanceBodyMesh,
            m_whiteMaterial,
            bodyTransform.toMatrix(),
            glm::vec4(0.75f, seedFrac, 1.0f, 1.0f),
            KIND_ICE_CRYSTAL
        );

        // Nucleo cristalino interior rotado 30 grados para mayor riqueza visual
        Transform coreTransform = bodyTransform;
        coreTransform.scale *= glm::vec3(0.62f, 0.62f, 0.82f);
        coreTransform.rot.z += GX_PI / 6.0f;
        renderer.submit(
            m_lanceBodyMesh,
            m_whiteMaterial,
            coreTransform.toMatrix(),
            glm::vec4(0.95f, gx_fract(seedFrac + 0.37f), 1.0f, 1.0f),
            KIND_ICE_CRYSTAL
        );

        // Luz puntual fria que acompana al Ice Lance en vuelo
        renderer.submitLight({
            e.transform.pos,
            4.5f,
            ICE_LIGHT_COLOR,
            1.25f * grow
        });
    }

    // 2. Estela de vuelo: sigue la trayectoria REAL (lineal, parabolica o con rebotes)
    // recorriendo el historial de los ultimos 36 ticks (0.6 segundos)
    const int startFrame = (std::max)(0, currentFrame - TRAIL_TICKS + 1);
    for (int t = startFrame; t <= currentFrame; ++t) {
        const int lagTicks = currentFrame - t;
        const float puffAge = static_cast<float>(lagTicks) * CombatSimulation::SECONDS_PER_TICK;
        const float frac = gx_clamp(puffAge / TRAIL_SPAN, 0.0f, 1.0f);

        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& sample = combatFrames[t][eIdx];
            if (!sample.isIceLance)
                continue;

            const float elemIdx = static_cast<float>(t * 17 + (sample.poolIndex >= 0 ? sample.poolIndex * 53 : static_cast<int>(eIdx) * 29));
            const float h1 = gx_hash11(elemIdx * 1.23f + 19.0f);
            const float h2 = gx_hash11(elemIdx * 2.47f + 23.0f);

            // Posicion real por donde paso el proyectil en el tick t + deriva fria (glacilux_trail.gdshader)
            const glm::vec3 randOffset = gx_hash33(glm::vec3(elemIdx, 19.0f, 3.0f)) - glm::vec3(0.5f);
            glm::vec3 puffPos = sample.transform.pos + randOffset * TRAIL_SPREAD * (0.22f + frac);
            puffPos.y -= TRAIL_SINK * puffAge * puffAge;

            const float baseAlpha = 1.0f - gx_smoothstep(0.12f, 1.0f, frac);
            if (baseAlpha <= 0.01f)
                continue;

            // Niebla fria (part 0)
            const float mistSize = TRAIL_SIZE * (0.72f + 0.56f * h1) * (0.55f + 1.10f * frac + puffAge * 0.35f);
            const float mistAlpha = baseAlpha * 0.54f;
            renderer.submit(
                m_mistQuadMesh,
                m_whiteMaterial,
                makeBillboardMatrix(puffPos, camRight, camUp, camForward, mistSize),
                glm::vec4(1.0f, h1, 1.0f, mistAlpha),
                KIND_COLD_MIST
            );

            // Sparkles / destellos (part 1)
            const float ph = static_cast<float>(timeSeconds) * SPARKLE_TWINKLE * (0.6f + h2) + h1 * 41.0f;
            const float sSin = (std::max)(std::sin(ph), 0.0f);
            const float twinkle = sSin * sSin * sSin;
            const float sparkleAlpha = baseAlpha * twinkle * 0.95f;
            if (sparkleAlpha > 0.03f) {
                const glm::vec3 spOffset = (gx_hash33(glm::vec3(elemIdx, 41.0f, 7.0f)) - glm::vec3(0.5f)) * (TRAIL_SPREAD * 0.7f);
                const glm::vec3 spPos = puffPos + spOffset;
                const float dist = glm::length(spPos - camPos);
                const float spSize = (std::max)(TRAIL_SIZE * 0.38f * (0.7f + 0.6f * h2), 0.0045f * dist);
                renderer.submit(
                    m_sparkleQuadMesh,
                    m_whiteMaterial,
                    makeBillboardMatrix(spPos, camRight, camUp, camForward, spSize),
                    glm::vec4(1.0f, h2, 1.0f, sparkleAlpha),
                    KIND_SPARKLE
                );
            }
        }
    }
}

void IceLanceVFX::submitImpacts(
    Renderer& renderer,
    const std::vector<ImpactVFX>& impacts,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    for (const ImpactVFX& impact : impacts) {
        const float age = static_cast<float>(currentFrame - impact.startTick) * CombatSimulation::SECONDS_PER_TICK;
        if (age < 0.0f || age > impact.duration)
            continue;

        if (impact.type == "fire_ball" || impact.type == "fire_ball_bounce") {
            const bool isBounceOnly = (impact.type == "fire_ball_bounce");
            submitFireImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward);
        } else if (impact.type == "lightning_bolt" || impact.type == "lightning_bolt_bounce") {
            const bool isBounceOnly = (impact.type == "lightning_bolt_bounce");
            submitLightningImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward, timeSeconds);
        } else if (impact.type == "earth_spike" || impact.type == "earth_spike_bounce") {
            const bool isBounceOnly = (impact.type == "earth_spike_bounce");
            submitEarthImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward);
        } else if (impact.type == "wind_blade" || impact.type == "wind_blade_bounce") {
            const bool isBounceOnly = (impact.type == "wind_blade_bounce");
            submitWindImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward);
        } else if (impact.type == "water_jet" || impact.type == "water_jet_bounce") {
            const bool isBounceOnly = (impact.type == "water_jet_bounce");
            submitWaterImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward);
        } else if (impact.type == "dagger_barrage" || impact.type == "dagger_barrage_bounce") {
            const bool isBounceOnly = (impact.type == "dagger_barrage_bounce");
            submitDaggerImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward);
        } else {
            const bool isBounceOnly = (impact.type == "ice_lance_bounce");
            submitSingleImpact(renderer, impact, age, isBounceOnly, camPos, camRight, camUp, camForward, timeSeconds);
        }
    }
}

void IceLanceVFX::submitSingleImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    // Parametros de impact_shards (glacilux_impact_3d.gd) adaptados a la escala visual de BattleMage
    const float impactRadius  = isBounceOnly ? 0.55f : 1.45f;
    const float spreadTime    = isBounceOnly ? 0.12f : 0.40f;
    const float holdTime      = isBounceOnly ? 0.18f : 1.10f;
    const int   crownCount    = isBounceOnly ? 0     : 14;
    const float crownSize     = 0.78f;
    const int   shardCount    = isBounceOnly ? 7     : 56;
    const float shardForce    = isBounceOnly ? 3.2f  : 6.2f;
    const float shardLifetime = isBounceOnly ? 0.42f : 1.35f;
    const float shardGravity  = 9.5f;
    const float shardBaseSize = isBounceOnly ? 0.11f : 0.18f;
    const int   mistCount     = isBounceOnly ? 5     : 18;
    const int   sparkleCount  = isBounceOnly ? 8     : 24;
    const float frostOpacity  = isBounceOnly ? 0.65f : 0.92f;
    constexpr float GROUND_Y  = 0.0f;

    const glm::vec3 normal = glm::length(impact.normal) > 0.0001f
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat3 surfaceBasis = gx_basis_from_normal(normal);
    const float seedSalt = static_cast<float>((impact.startTick * 73) % 997) +
        std::abs(impact.position.x * 13.0f + impact.position.z * 17.0f);

    // 1. Parche de escarcha orientado segun la normal de colision (glacilux_ground.gdshader)
    const float fill = gx_clamp(age / (std::max)(spreadTime, 0.02f), 0.0f, 1.0f);
    const float fade = 1.0f - gx_clamp((age - spreadTime - holdTime) / (isBounceOnly ? 0.20f : 0.80f), 0.0f, 1.0f);
    if (fill > 0.01f && fade > 0.01f) {
        const float patchRad = impactRadius * 0.95f * std::sqrt(fill);
        const glm::vec3 liftedPos = impact.position + normal * 0.025f;
        glm::mat4 frostModel(1.0f);
        frostModel[0] = glm::vec4(surfaceBasis[0] * patchRad, 0.0f);
        frostModel[1] = glm::vec4(surfaceBasis[1] * patchRad, 0.0f);
        frostModel[2] = glm::vec4(surfaceBasis[2] * patchRad, 0.0f);
        frostModel[3] = glm::vec4(liftedPos, 1.0f);

        const float frostRand = gx_hash11(seedSalt + 17.0f);
        renderer.submit(
            m_frostQuadMesh,
            m_whiteMaterial,
            frostModel,
            glm::vec4(fill, frostRand, 1.0f, frostOpacity * fade),
            KIND_FROST_PATCH
        );
    }

    // 2. Corona de cristales de hielo orientada con la normal del impacto (glacilux_spike.gdshader)
    const float crownStagger   = spreadTime * 0.35f;
    const float crownRiseTime  = (std::max)(spreadTime * 0.35f, 0.05f);
    const float crownHoldTime  = holdTime * 0.60f;
    const float crownLeaveTime = (std::max)(holdTime * 0.50f, 0.25f);

    for (int i = 0; i < crownCount; ++i) {
        const float fi = static_cast<float>(i);
        const float r1 = gx_hash11(fi * 1.43f + seedSalt + 71.0f);
        const float r2 = gx_hash11(fi * 2.91f + seedSalt + 83.0f);
        const float r3 = gx_hash11(fi * 3.73f + seedSalt + 97.0f);

        const float staggerOrder = r1 * 0.22f;
        const float spikeAge = age - staggerOrder * crownStagger;
        if (spikeAge <= 0.0f)
            continue;

        const float g = gx_overshoot(gx_clamp(spikeAge / crownRiseTime, 0.0f, 1.0f), 0.70f);
        const float leave = gx_clamp((spikeAge - crownRiseTime - crownHoldTime) / crownLeaveTime, 0.0f, 1.0f);
        if (g < 0.02f || leave >= 1.0f)
            continue;

        const float angle = GX_TAU * fi / static_cast<float>(crownCount) + r1 * 0.40f;
        const float ringDist = impactRadius * (0.24f + r2 * 0.28f);
        const glm::vec3 localOut(std::cos(angle), 0.0f, std::sin(angle));
        const glm::vec3 worldOut = surfaceBasis * localOut;
        const glm::vec3 spikePos = impact.position + worldOut * ringDist;

        // Cada cristal de la corona se inclina hacia fuera alrededor de la normal de impacto
        const glm::vec3 spikeUp = glm::normalize(normal * 1.55f + worldOut);
        const glm::mat3 spikeBasis = gx_basis_from_normal(spikeUp);

        const float shrink = 1.0f - leave;
        const float sizeScale = (0.65f + r3 * 0.65f);
        const float hh = crownSize * sizeScale * g * shrink;
        const float rr = crownSize * 0.22f * (0.80f + 0.40f * r2) * g * shrink;

        glm::mat4 spikeModel(1.0f);
        spikeModel[0] = glm::vec4(spikeBasis[0] * rr, 0.0f);
        spikeModel[1] = glm::vec4(spikeBasis[1] * hh, 0.0f);
        spikeModel[2] = glm::vec4(spikeBasis[2] * rr, 0.0f);
        spikeModel[3] = glm::vec4(spikePos, 1.0f);

        renderer.submit(
            m_crownSpikeMesh,
            m_whiteMaterial,
            spikeModel,
            glm::vec4(0.80f, r1, 1.0f, 1.0f - leave * 0.85f),
            KIND_ICE_CRYSTAL
        );
    }

    // 3. Fragmentos / Shards con trayectoria balistica analitica, gravedad y rebotes (glacilux_shards.gdshader)
    for (int i = 0; i < shardCount; ++i) {
        const float fi = static_cast<float>(i);
        const float h1 = gx_hash11(fi * 1.17f + seedSalt + 109.0f);
        const float h2 = gx_hash11(fi * 2.53f + seedSalt + 113.0f);
        const float life = (std::max)(shardLifetime * (0.75f + 0.50f * h1), 0.08f);
        if (age > life)
            continue;

        const glm::vec3 coneDir = sampleDirectionInCone(i, normal, 135.0f, seedSalt + 97.0f);
        const glm::vec3 wp = impact.position + coneDir * (impactRadius * 0.18f * std::pow(h1, 0.35f));
        const glm::vec3 launchDir = glm::normalize(glm::mix(coneDir, normal, 0.42f) + gx_unit_from_index(fi, seedSalt) * 0.30f);
        glm::vec3 v = launchDir * (shardForce * (0.55f + 0.90f * h2));

        // Trayectoria balistica analitica con hasta 3 rebotes sobre el suelo (y = 0)
        glm::vec3 p = wp;
        float t = age;
        constexpr float RESTITUTION = 0.34f;
        constexpr float FRICTION    = 0.72f;
        for (int b = 0; b < 3; ++b) {
            const float disc = v.y * v.y + 2.0f * shardGravity * (p.y - GROUND_Y);
            if (disc <= 0.0f)
                break;
            const float th = (v.y + std::sqrt(disc)) / (std::max)(shardGravity, 0.0001f);
            if (th <= 0.0005f || th >= t)
                break;
            p += v * th;
            p.y = GROUND_Y;
            v.y = -(v.y - shardGravity * th) * RESTITUTION;
            v.x *= FRICTION;
            v.z *= FRICTION;
            t -= th;
        }
        p += v * t;
        p.y -= 0.5f * shardGravity * t * t;
        p.y = (std::max)(p.y, GROUND_Y + 0.02f);

        // Si el impacto fue contra una pared o personaje, mantener los shards delante del plano de impacto
        const float planeDist = glm::dot(p - impact.position, normal);
        if (planeDist < 0.02f) {
            p += normal * (0.02f - planeDist);
        }

        const float u = age / life;
        const float settle = 1.0f - gx_smoothstep(0.35f, 0.85f, u);
        const glm::mat3 rot = gx_rot_axis(
            gx_unit_from_index(fi, seedSalt + 9.0f),
            7.0f * (0.5f + h1) * age * (0.3f + 0.7f * settle)
        );
        const float shrink = 1.0f - gx_smoothstep(0.72f, 1.0f, u);
        const float sz = shardBaseSize * (0.60f + 0.80f * h2) * shrink;
        const float shardFade = gx_smoothstep(0.0f, 0.04f, u) * (1.0f - gx_smoothstep(0.74f, 1.0f, u));
        if (shardFade <= 0.01f || sz <= 0.002f)
            continue;

        const glm::mat3 mRot = rot * surfaceBasis;
        glm::mat4 shardModel(1.0f);
        shardModel[0] = glm::vec4(mRot[0] * sz, 0.0f);
        shardModel[1] = glm::vec4(mRot[1] * sz, 0.0f);
        shardModel[2] = glm::vec4(mRot[2] * sz, 0.0f);
        shardModel[3] = glm::vec4(p, 1.0f);

        const MeshId shardMesh = (i % 3 == 0) ? m_shardWedgeMesh : m_shardBoxMesh;
        renderer.submit(
            shardMesh,
            m_whiteMaterial,
            shardModel,
            glm::vec4(0.85f, h1, 1.0f, shardFade),
            KIND_ICE_CRYSTAL
        );
    }

    // 4. Niebla fria del impacto (glacilux_mist.gdshader)
    const float mistBaseLife = (std::max)(spreadTime * 2.2f, 0.75f);
    const float mistSpeed    = impactRadius * 1.55f + shardForce * 0.12f;
    for (int i = 0; i < mistCount; ++i) {
        const float fi = static_cast<float>(i);
        const float h1 = gx_hash11(fi * 1.31f + seedSalt + 131.0f);
        const float h2 = gx_hash11(fi * 2.17f + seedSalt + 138.0f);
        const float h3 = gx_hash11(fi * 3.71f + seedSalt + 144.0f);

        const float puffAge = age - 0.05f * h1 * 0.35f;
        const float life = mistBaseLife * (0.70f + 0.60f * h2) * 0.85f;
        const float u = puffAge / (std::max)(life, 0.001f);
        if (u < 0.0f || u > 1.0f)
            continue;

        const glm::vec3 coneDir = sampleDirectionInCone(i, normal, 150.0f, seedSalt + 131.0f);
        glm::vec3 pos = impact.position + coneDir * (impactRadius * 0.25f * std::pow(h1, 0.35f));
        pos += coneDir * (mistSpeed * puffAge * 0.85f);
        pos += normal * (0.04f + impactRadius * 0.35f * u);
        pos.y += mistSpeed * puffAge * 0.22f - 0.70f * puffAge * puffAge * 0.5f;
        pos.y = (std::max)(pos.y, GROUND_Y + 0.08f);

        const float sz = glm::mix(impactRadius * 0.32f, impactRadius * 1.05f, u) * (0.75f + 0.55f * h3);
        const float alpha = gx_smoothstep(0.0f, 0.18f, u) * (1.0f - gx_smoothstep(0.48f, 1.0f, u)) * 0.52f;
        if (alpha <= 0.01f)
            continue;

        renderer.submit(
            m_mistQuadMesh,
            m_whiteMaterial,
            makeBillboardMatrix(pos, camRight, camUp, camForward, sz),
            glm::vec4(1.0f, h1, 1.0f, alpha),
            KIND_COLD_MIST
        );
    }

    // 5. Sparkles del impacto (glacilux_sparkle.gdshader)
    const float sparkleLife = spreadTime + holdTime;
    for (int i = 0; i < sparkleCount; ++i) {
        const float fi = static_cast<float>(i);
        const float h = gx_hash11(fi * 1.91f + seedSalt + 149.0f);
        const float bage = age - h * spreadTime * 0.65f;
        if (bage <= 0.0f || bage >= sparkleLife)
            continue;

        float born = gx_smoothstep(0.0f, 0.25f, bage);
        born *= 1.0f - gx_smoothstep(sparkleLife * 0.55f, sparkleLife, bage);

        const float ph = static_cast<float>(timeSeconds) * 6.5f * (0.6f + h) + h * 41.0f;
        const float sSin = (std::max)(std::sin(ph), 0.0f);
        const float tw = sSin * sSin * sSin * sSin;
        const float a = born * tw;
        if (a < 0.02f)
            continue;

        const glm::vec3 coneDir = sampleDirectionInCone(i, normal, 160.0f, seedSalt + 149.0f);
        glm::vec3 wp = impact.position + coneDir * (impactRadius * (0.25f + 0.65f * h)) + normal * 0.08f;
        wp.y = (std::max)(wp.y, GROUND_Y + 0.06f);
        const float dist = glm::length(wp - camPos);
        const float sz = (std::max)(impactRadius * 0.12f * (0.6f + 0.8f * h), 0.005f * dist);

        renderer.submit(
            m_sparkleQuadMesh,
            m_whiteMaterial,
            makeBillboardMatrix(wp, camRight, camUp, camForward, sz),
            glm::vec4(1.0f, h, 1.0f, a),
            KIND_SPARKLE
        );
    }

    // 6. Destello de luz breve en el momento del impacto (_update_light en glacilux_impact_3d.gd)
    const float flashDuration = (std::max)(spreadTime * 1.4f, 0.12f);
    if (age < flashDuration) {
        const float flash = (std::max)(1.0f - age / flashDuration, 0.0f);
        renderer.submitLight({
            impact.position + normal * 0.40f,
            isBounceOnly ? 3.5f : 6.8f,
            ICE_LIGHT_COLOR,
            (isBounceOnly ? 1.4f : 3.2f) * flash * flash
        });
    }
}

void IceLanceVFX::submitFireCastAndFlight(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    // =========================================================================
    // FASE 1 — FIRE CAST (fire_cast.tscn)
    // Detectado automaticamente en el primer tick de vida del fire_ball (animationAge == 0)
    // =========================================================================
    constexpr int CAST_WINDOW_TICKS = 60; // ~1.0 s de duracion visible del cast
    const int castStartFrame = (std::max)(0, currentFrame - CAST_WINDOW_TICKS);
    for (int t = castStartFrame; t <= currentFrame; ++t) {
        const float castAge = static_cast<float>(currentFrame - t) * CombatSimulation::SECONDS_PER_TICK;
        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& e = combatFrames[t][eIdx];
            if (!e.isFireBall || e.animationAge < 0.0 || e.animationAge > 0.008)
                continue;

            const glm::vec3 castPos = e.transform.pos;
            const glm::vec3 castDir = directionFromPitchYaw(e.transform.rot);
            const glm::mat3 castBasis = gx_basis_from_normal(castDir);
            const float seedSalt = static_cast<float>(t * 31 + (e.poolIndex >= 0 ? e.poolIndex * 67 : static_cast<int>(eIdx) * 43));

            // 1.1 Flash inicial (lifetime = 0.10 s, size = 1.1 -> escalado a 1.85)
            if (castAge <= 0.12f) {
                const float u = castAge / 0.12f;
                const float curve = (u < 0.3f) ? glm::mix(0.5f, 1.0f, u / 0.3f) : glm::mix(1.0f, 0.3f, (u - 0.3f) / 0.7f);
                const float alpha = 1.0f - u;
                const glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.95f, 0.80f), glm::vec3(1.0f, 0.50f, 0.10f), u);
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeBillboardMatrix(castPos, camRight, camUp, camForward, 1.95f * curve),
                    glm::vec4(col, alpha),
                    KIND_FIRE_FLASH
                );
            }

            // 1.2 Ring (lifetime = 0.25 s, facing = Mesh perpendicular a la direccion de disparo)
            if (castAge <= 0.25f) {
                const float u = castAge / 0.25f;
                const float ringRad = glm::mix(0.22f, 0.90f, u);
                const float ringScale = 1.15f;
                const glm::vec3 ringPos = castPos + castDir * 0.15f;
                glm::mat4 ringModel(1.0f);
                ringModel[0] = glm::vec4(castBasis[0] * ringScale, 0.0f);
                ringModel[1] = glm::vec4(castBasis[1] * ringScale, 0.0f);
                ringModel[2] = glm::vec4(castBasis[2] * ringScale, 0.0f);
                ringModel[3] = glm::vec4(ringPos, 1.0f);
                renderer.submit(
                    m_fireDecalMesh,
                    m_whiteMaterial,
                    ringModel,
                    glm::vec4(ringRad, gx_hash11(seedSalt + 5.0f), 0.85f, (1.0f - u) * 0.95f),
                    KIND_FIRE_DECAL
                );
            }

            // 1.3 Flames (amount = 14, lifetime = 0.38 s, spread = 28 deg, speed = 3.0..6.5, damping = 10)
            if (castAge <= 0.40f) {
                for (int i = 0; i < 14; ++i) {
                    const float fi = static_cast<float>(i);
                    const float h1 = gx_hash11(fi * 1.37f + seedSalt + 11.0f);
                    const float h2 = gx_hash11(fi * 2.59f + seedSalt + 19.0f);
                    const float life = 0.38f * (1.0f - 0.30f * h1);
                    if (castAge > life) continue;
                    const float u = castAge / life;
                    const glm::vec3 dir = sampleDirectionInCone(i, castDir, 56.0f, seedSalt + 23.0f);
                    const float spd = glm::mix(3.2f, 6.8f, h2);
                    const glm::vec3 pos = castPos + dir * dampedDistance(spd, 9.5f, castAge);
                    const float scaleCurve = (u < 0.35f) ? glm::mix(0.4f, 1.0f, u / 0.35f) : glm::mix(1.0f, 0.2f, (u - 0.35f) / 0.65f);
                    const float sz = 0.72f * glm::mix(0.8f, 1.25f, h1) * scaleCurve;
                    const float alpha = (1.0f - gx_smoothstep(0.55f, 1.0f, u)) * 0.92f;
                    const float angle = h2 * GX_TAU + (h1 - 0.5f) * 3.0f * castAge;
                    renderer.submit(
                        m_fireFlameMesh,
                        m_whiteMaterial,
                        makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, sz, angle),
                        glm::vec4(evalFireFlameRamp(u, h1), alpha),
                        KIND_FIRE_FLAME
                    );
                }
            }

            // 1.4 Embers (amount = 18, lifetime = 0.60 s, spread = 35 deg, speed = 3..7, gravity_y = -3)
            if (castAge <= 0.62f) {
                for (int i = 0; i < 18; ++i) {
                    const float fi = static_cast<float>(i);
                    const float h1 = gx_hash11(fi * 1.71f + seedSalt + 31.0f);
                    const float h2 = gx_hash11(fi * 2.83f + seedSalt + 41.0f);
                    const float life = 0.60f * (1.0f - 0.40f * h1);
                    if (castAge > life) continue;
                    const float u = castAge / life;
                    const glm::vec3 dir = sampleDirectionInCone(i, castDir, 70.0f, seedSalt + 47.0f);
                    const float spd = glm::mix(3.2f, 7.2f, h2);
                    const float dmp = glm::mix(4.0f, 7.0f, h1);
                    glm::vec3 pos = castPos + dir * dampedDistance(spd, dmp, castAge);
                    pos.y -= 0.5f * 3.0f * castAge * castAge;
                    const glm::vec3 vel = dir * (spd * std::exp(-dmp * castAge)) + glm::vec3(0.0f, -3.0f * castAge, 0.0f);
                    const float scale = glm::mix(0.55f, 1.45f, h1) * (1.0f - u);
                    const glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.95f, 0.70f), glm::vec3(1.0f, 0.45f, 0.10f), u);
                    renderer.submit(
                        m_fireFlashMesh,
                        m_whiteMaterial,
                        makeAlignedBillboardMatrix(pos, vel, camPos, camRight, 0.09f * scale, 0.34f * scale),
                        glm::vec4(col, 1.0f - u),
                        KIND_FIRE_FLASH
                    );
                }
            }

            // 1.5 Smoke (amount = 6, delay = 0.05 s, lifetime = 0.90 s, gravity_y = +0.8)
            if (castAge >= 0.05f && castAge <= 0.98f) {
                for (int i = 0; i < 6; ++i) {
                    const float fi = static_cast<float>(i);
                    const float h1 = gx_hash11(fi * 1.93f + seedSalt + 59.0f);
                    const float h2 = gx_hash11(fi * 3.11f + seedSalt + 67.0f);
                    const float pAge = castAge - 0.05f - h1 * 0.12f;
                    const float life = 0.85f * (1.0f - 0.25f * h2);
                    if (pAge <= 0.0f || pAge > life) continue;
                    const float u = pAge / life;
                    const glm::vec3 dir = sampleDirectionInCone(i, castDir, 60.0f, seedSalt + 71.0f);
                    const float spd = glm::mix(1.0f, 2.0f, h2);
                    glm::vec3 pos = castPos + dir * dampedDistance(spd, 3.5f, pAge);
                    pos.y += 0.8f * pAge;
                    const float sz = 0.65f * glm::mix(0.8f, 1.25f, h1) * glm::mix(0.4f, 1.6f, u);
                    const float alpha = gx_smoothstep(0.0f, 0.25f, u) * (1.0f - gx_smoothstep(0.35f, 1.0f, u)) * 0.42f;
                    const glm::vec3 smokeCol = glm::mix(glm::vec3(0.32f, 0.23f, 0.18f), glm::vec3(0.17f, 0.15f, 0.16f), u);
                    renderer.submit(
                        m_fireSmokeMesh,
                        m_whiteMaterial,
                        makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, sz, h2 * GX_TAU + pAge * 1.2f),
                        glm::vec4(smokeCol, alpha),
                        KIND_FIRE_SMOKE
                    );
                }
            }

            // 1.6 Luz de lanzamiento (flash = 0.35 s, energy = 3.0)
            if (castAge <= 0.35f) {
                const float lFade = 1.0f - castAge / 0.35f;
                renderer.submitLight({
                    castPos,
                    4.5f,
                    FIRE_CAST_LIGHT_COLOR,
                    3.0f * lFade * lFade
                });
            }
        }
    }

    // =========================================================================
    // FASE 2 — FIRE PROJECTILE EN VUELO (fire_projectile.tscn)
    // =========================================================================
    for (size_t eIdx = 0; eIdx < combatFrames[currentFrame].size(); ++eIdx) {
        const Entidad& e = combatFrames[currentFrame][eIdx];
        if (!e.isFireBall)
            continue;

        const glm::vec3 pos = e.transform.pos;
        const float age = static_cast<float>(e.animationAge >= 0.0 ? e.animationAge : timeSeconds);
        const float seedSalt = static_cast<float>(e.poolIndex >= 0 ? e.poolIndex * 47 + 13 : static_cast<int>(eIdx) * 31 + 13);

        // 2.1 Halo (QuadMesh 0.9 -> escalado a 1.85, color (1, 0.3, 0.05, 0.25))
        renderer.submit(
            m_fireFlameMesh,
            m_whiteMaterial,
            makeBillboardMatrix(pos, camRight, camUp, camForward, 1.85f),
            glm::vec4(1.0f, 0.35f, 0.06f, 0.36f),
            KIND_FIRE_FLAME
        );

        // 2.2 Core (SphereMesh con sp_orb.gdshader style = 1 Flame)
        Transform orbTransform;
        orbTransform.pos = pos;
        orbTransform.rot = e.transform.rot;
        orbTransform.scale = glm::vec3(0.46f);
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            orbTransform.toMatrix(),
            glm::vec4(1.0f, age * 2.2f, 0.4f, 0.96f),
            KIND_FIRE_ORB
        );

        // Capa exterior envolvente del Orb para darle volumen de plasma ardiente
        orbTransform.scale = glm::vec3(0.64f);
        renderer.submit(
            m_fireOrbMesh,
            m_whiteTexture,
            orbTransform.toMatrix(),
            glm::vec4(1.0f, age * 2.2f + 1.7f, 0.2f, 0.68f),
            KIND_FIRE_ORB
        );

        // 2.3 CoreFlames (amount = 8, lifetime = 0.25 s en bucle continuo alrededor del nucleo)
        for (int i = 0; i < 8; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.53f + seedSalt);
            const float h2 = gx_hash11(fi * 2.77f + seedSalt + 7.0f);
            const float cycle = gx_fract(age / 0.25f + fi / 8.0f);
            const glm::vec3 offsetDir = gx_unit_from_index(fi, seedSalt + std::floor(age / 0.25f + fi / 8.0f));
            const glm::vec3 flamePos = pos + offsetDir * (0.08f + 0.28f * cycle);
            const float scaleCurve = (cycle < 0.4f)
                ? glm::mix(0.6f, 1.0f, cycle / 0.4f)
                : glm::mix(1.0f, 0.4f, (cycle - 0.4f) / 0.6f);
            const float sz = 1.05f * glm::mix(0.8f, 1.25f, h1) * scaleCurve;
            const float alpha = (1.0f - gx_smoothstep(0.60f, 1.0f, cycle)) * 0.85f;
            const float spinDir = (i % 2 == 0) ? 1.0f : -1.0f;
            const float angle = h2 * GX_TAU + spinDir * 4.0f * cycle;
            renderer.submit(
                m_fireFlameMesh,
                m_whiteTexture,
                makeRotatedBillboardMatrix(flamePos, camRight, camUp, camForward, sz, angle),
                glm::vec4(evalFireFlameRamp(cycle * 0.85f, h1), alpha),
                KIND_FIRE_FLAME
            );
        }

        // 2.4 Luz puntual calida con parpadeo (sp_flicker = 0.45 en fire_projectile.tscn)
        const float wave = std::sin(age * 23.0f) * 0.5f + std::sin(age * 7.3f + 1.7f) * 0.5f;
        const float energy = 2.2f * (1.0f - 0.225f + 0.225f * wave);
        renderer.submitLight({
            pos,
            5.5f,
            FIRE_LIGHT_COLOR,
            energy
        });
    }

    // =========================================================================
    // 2.5 ESTELA Y PARTICULAS DE VUELO (FlameTrail, HotTrail, Flames, Smoke, Embers)
    // Reutiliza las posiciones REALES del replay (curva parabolica, gravedad y rebotes)
    // =========================================================================
    constexpr int FIRE_TRAIL_TICKS = 54; // 0.90 s a 60 Hz (cubre Smoke 0.9s, Embers 0.8s, Flames 0.35s, Trails 0.28s)
    const int trailStartFrame = (std::max)(0, currentFrame - FIRE_TRAIL_TICKS);
    for (int t = trailStartFrame; t <= currentFrame; ++t) {
        const int lagTicks = currentFrame - t;
        const float lagAge = static_cast<float>(lagTicks) * CombatSimulation::SECONDS_PER_TICK;

        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& sample = combatFrames[t][eIdx];
            if (!sample.isFireBall)
                continue;

            const glm::vec3 histPos = sample.transform.pos;
            const glm::vec3 histDir = directionFromPitchYaw(sample.transform.rot);
            const float elemIdx = static_cast<float>(t * 19 + (sample.poolIndex >= 0 ? sample.poolIndex * 61 : static_cast<int>(eIdx) * 37));
            const float h1 = gx_hash11(elemIdx * 1.31f + 13.0f);
            const float h2 = gx_hash11(elemIdx * 2.63f + 29.0f);
            const float h3 = gx_hash11(elemIdx * 3.89f + 43.0f);

            // A) HotTrail (lifetime = 0.09..0.14 s, width = 0.24 -> 0.45): nucleo incandescente de la estela
            if (lagTicks > 0 && lagAge <= 0.14f) {
                const float u = lagAge / 0.14f;
                const float w = 0.44f * (1.0f - 0.65f * u);
                const glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.88f, 0.55f), glm::vec3(1.0f, 0.38f, 0.05f), u);
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeAlignedBillboardMatrix(histPos, histDir, camPos, camRight, w, 0.65f),
                    glm::vec4(col, (1.0f - u) * 0.92f),
                    KIND_FIRE_FLASH
                );
            }

            // B) FlameTrail (lifetime = 0.28 s, width = 0.62 -> 0.95) + Flames (lifetime = 0.35 s, gravity_y = +1.5)
            if (lagAge <= 0.36f) {
                const float u = lagAge / 0.36f;
                const float widthCurve = (u < 0.25f)
                    ? glm::mix(0.8f, 1.0f, u / 0.25f)
                    : glm::mix(1.0f, 0.35f, (u - 0.25f) / 0.75f);

                // Segmento de ribbon FlameTrail alineado con la tangente real de la trayectoria
                if (lagTicks > 0 && lagAge <= 0.28f) {
                    const float tu = lagAge / 0.28f;
                    renderer.submit(
                        m_fireFlameMesh,
                        m_whiteMaterial,
                        makeAlignedBillboardMatrix(histPos, histDir, camPos, camRight, 0.92f * widthCurve, 0.78f),
                        glm::vec4(evalFireFlameRamp(tu, h1), (1.0f - tu * 0.85f) * 0.82f),
                        KIND_FIRE_FLAME
                    );
                }

                // Particula de llama (Flames: gravity_y = +1.5, damping = 16, scale 1.0 -> 0.2)
                const glm::vec3 randOff = (gx_hash33(glm::vec3(elemIdx, 17.0f, 5.0f)) - glm::vec3(0.5f)) * 0.45f;
                glm::vec3 flamePos = histPos + randOff * (0.25f + u);
                flamePos.y += 1.5f * lagAge * lagAge * 0.5f;
                const float flameSz = 0.86f * glm::mix(0.8f, 1.25f, h1) * glm::mix(1.0f, 0.22f, u);
                const float flameAlpha = (1.0f - gx_smoothstep(0.35f, 1.0f, u)) * 0.75f;
                const float angle = h2 * GX_TAU + (h1 - 0.5f) * 4.0f * lagAge;
                renderer.submit(
                    m_fireFlameMesh,
                    m_whiteMaterial,
                    makeRotatedBillboardMatrix(flamePos, camRight, camUp, camForward, flameSz, angle),
                    glm::vec4(evalFireFlameRamp(u, h2), flameAlpha),
                    KIND_FIRE_FLAME
                );
            }

            // C) Embers (amount = 30, lifetime = 0.80 s, gravity_y = +1.2, turbulence)
            if (lagAge <= 0.80f) {
                const float emberLife = 0.80f * (0.65f + 0.35f * h1);
                if (lagAge <= emberLife) {
                    const float u = lagAge / emberLife;
                    const glm::vec3 spreadDir = (gx_hash33(glm::vec3(elemIdx, 31.0f, 9.0f)) - glm::vec3(0.5f)) * 2.0f;
                    const float spd = glm::mix(0.6f, 3.2f, h2);
                    glm::vec3 emberPos = histPos + spreadDir * dampedDistance(spd, 4.5f, lagAge);
                    emberPos.y += 1.2f * lagAge * lagAge * 0.5f;
                    // Turbulencia sinusoidal como turbulence_enabled en fire_projectile.tscn
                    emberPos.x += std::sin(lagAge * 9.0f + h1 * 17.0f) * 0.14f * u;
                    emberPos.z += std::cos(lagAge * 8.5f + h2 * 19.0f) * 0.14f * u;
                    const float emberSz = 0.16f * glm::mix(0.55f, 1.45f, h3) * glm::mix(1.0f, 0.2f, u);
                    const glm::vec3 emberCol = (u < 0.6f)
                        ? glm::mix(glm::vec3(1.0f, 0.85f, 0.45f), glm::vec3(1.0f, 0.42f, 0.08f), u / 0.6f)
                        : glm::mix(glm::vec3(1.0f, 0.42f, 0.08f), glm::vec3(0.8f, 0.12f, 0.0f), (u - 0.6f) / 0.4f);
                    renderer.submit(
                        m_fireFlashMesh,
                        m_whiteMaterial,
                        makeBillboardMatrix(emberPos, camRight, camUp, camForward, emberSz),
                        glm::vec4(emberCol, 1.0f - gx_smoothstep(0.6f, 1.0f, u)),
                        KIND_FIRE_FLASH
                    );
                }
            }

            // D) Smoke (amount = 20, lifetime = 0.90 s, gravity_y = +1.0, scale 0.5 -> 1.8)
            if (t % 2 == 0 && lagAge <= 0.90f) {
                const float smokeLife = 0.90f * (0.75f + 0.25f * h2);
                if (lagAge <= smokeLife) {
                    const float u = lagAge / smokeLife;
                    // Gradient_37: el humo aparece detras de las llamas a partir de u ~ 0.18
                    float smokeAlpha = gx_smoothstep(0.15f, 0.42f, u) * (1.0f - gx_smoothstep(0.48f, 1.0f, u)) * 0.36f;
                    if (smokeAlpha > 0.01f) {
                        const glm::vec3 drift = (gx_hash33(glm::vec3(elemIdx, 53.0f, 11.0f)) - glm::vec3(0.5f)) * 0.65f;
                        glm::vec3 smokePos = histPos + drift * u;
                        smokePos.y += 1.0f * lagAge;
                        const float smokeSz = 0.72f * glm::mix(0.8f, 1.25f, h1) * glm::mix(0.5f, 1.8f, u);
                        const glm::vec3 smokeCol = glm::mix(glm::vec3(0.24f, 0.19f, 0.16f), glm::vec3(0.26f, 0.24f, 0.22f), u);
                        renderer.submit(
                            m_fireSmokeMesh,
                            m_whiteMaterial,
                            makeRotatedBillboardMatrix(smokePos, camRight, camUp, camForward, smokeSz, h3 * GX_TAU + lagAge * 0.9f),
                            glm::vec4(smokeCol, smokeAlpha),
                            KIND_FIRE_SMOKE
                        );
                    }
                }
            }
        }
    }
}

void IceLanceVFX::submitFireImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward) const
{
    // =========================================================================
    // FASE 3 — FIRE IMPACT (fire_impact.tscn, duration = 3.18 s) y rebote corto
    // Orientado usando la normal real de colision (suelo, pared o personaje)
    // =========================================================================
    constexpr float GROUND_Y = 0.0f;
    const glm::vec3 normal = glm::length(impact.normal) > 0.0001f
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat3 surfaceBasis = gx_basis_from_normal(normal);
    const float seedSalt = static_cast<float>((impact.startTick * 89) % 997) +
        std::abs(impact.position.x * 19.0f + impact.position.z * 23.0f);
    const float randSeed = gx_hash11(seedSalt + 13.0f);

    // 1. Marcas sobre la superficie orientadas con la normal (Scorch, Glow, Ring)
    if (!isBounceOnly) {
        // 1.1 Scorch (lifetime = 3.0 s, PlaneMesh 1.9, grow_time = 0.08, fade_out = 0.3)
        if (age <= 3.0f) {
            const float u = age / 3.0f;
            const float grow = 1.0f - std::pow(1.0f - gx_smoothstep(0.0f, 0.08f, u), 3.0f);
            const float fade = gx_smoothstep(0.0f, 0.01f, u) * (1.0f - gx_smoothstep(0.70f, 1.0f, u));
            const float hotCore = 1.0f - gx_smoothstep(0.0f, 0.45f, age);
            const float scorchScale = 1.45f * grow;
            const glm::vec3 scorchPos = impact.position + normal * 0.022f;
            glm::mat4 scorchModel(1.0f);
            scorchModel[0] = glm::vec4(surfaceBasis[0] * scorchScale, 0.0f);
            scorchModel[1] = glm::vec4(surfaceBasis[1] * scorchScale, 0.0f);
            scorchModel[2] = glm::vec4(surfaceBasis[2] * scorchScale, 0.0f);
            scorchModel[3] = glm::vec4(scorchPos, 1.0f);
            renderer.submit(
                m_fireDecalMesh,
                m_whiteMaterial,
                scorchModel,
                glm::vec4(hotCore, randSeed, 0.10f, 0.88f * fade),
                KIND_FIRE_DECAL
            );
        }

        // 1.2 Glow (lifetime = 1.2 s, grow_time = 0.1, fade_out = 0.7)
        if (age <= 1.2f) {
            const float u = age / 1.2f;
            const float grow = 1.0f - std::pow(1.0f - gx_smoothstep(0.0f, 0.10f, u), 3.0f);
            const float fade = 1.0f - gx_smoothstep(0.30f, 1.0f, u);
            const float glowScale = 1.55f * grow;
            const glm::vec3 glowPos = impact.position + normal * 0.030f;
            glm::mat4 glowModel(1.0f);
            glowModel[0] = glm::vec4(surfaceBasis[0] * glowScale, 0.0f);
            glowModel[1] = glm::vec4(surfaceBasis[1] * glowScale, 0.0f);
            glowModel[2] = glm::vec4(surfaceBasis[2] * glowScale, 0.0f);
            glowModel[3] = glm::vec4(glowPos, 1.0f);
            renderer.submit(
                m_fireDecalMesh,
                m_whiteMaterial,
                glowModel,
                glm::vec4(grow, randSeed, 0.45f, fade * 0.90f),
                KIND_FIRE_DECAL
            );
        }
    }

    // 1.3 Ring (lifetime = 0.40 s en impacto completo, 0.22 s en rebote, PlaneMesh 2.6)
    const float ringLife = isBounceOnly ? 0.22f : 0.40f;
    if (age <= ringLife) {
        const float u = age / ringLife;
        const float grow = 1.0f - std::pow(1.0f - gx_smoothstep(0.0f, 0.80f, u), 3.0f);
        const float ringRad = glm::mix(0.12f, 0.92f, grow);
        const float fade = 1.0f - gx_smoothstep(0.35f, 1.0f, u);
        const float ringScale = isBounceOnly ? 0.95f : 1.95f;
        const glm::vec3 ringPos = impact.position + normal * 0.038f;
        glm::mat4 ringModel(1.0f);
        ringModel[0] = glm::vec4(surfaceBasis[0] * ringScale, 0.0f);
        ringModel[1] = glm::vec4(surfaceBasis[1] * ringScale, 0.0f);
        ringModel[2] = glm::vec4(surfaceBasis[2] * ringScale, 0.0f);
        ringModel[3] = glm::vec4(ringPos, 1.0f);
        renderer.submit(
            m_fireDecalMesh,
            m_whiteMaterial,
            ringModel,
            glm::vec4(ringRad, randSeed, 0.85f, fade),
            KIND_FIRE_DECAL
        );
    }

    const glm::vec3 blastCenter = impact.position + normal * (isBounceOnly ? 0.20f : 0.42f);

    // 2. Flash (lifetime = 0.10..0.12 s, QuadMesh size = 2.0 -> 2.8)
    if (age <= 0.12f) {
        const float u = age / 0.12f;
        const float curve = (u < 0.25f) ? glm::mix(0.5f, 1.0f, u / 0.25f) : glm::mix(1.0f, 0.3f, (u - 0.25f) / 0.75f);
        const float flashSize = (isBounceOnly ? 1.4f : 2.8f) * curve;
        const glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.88f, 0.58f), glm::vec3(1.0f, 0.40f, 0.05f), u);
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            makeBillboardMatrix(blastCenter, camRight, camUp, camForward, flashSize),
            glm::vec4(col, 1.0f - u),
            KIND_FIRE_FLASH
        );
    }

    // 3. Core (amount = 6, lifetime = 0.35 s, QuadMesh size = 1.1 -> 1.55)
    const int coreCount = isBounceOnly ? 2 : 6;
    if (age <= 0.38f) {
        for (int i = 0; i < coreCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.41f + seedSalt + 17.0f);
            const float h2 = gx_hash11(fi * 2.73f + seedSalt + 29.0f);
            const float life = (isBounceOnly ? 0.22f : 0.35f) * (1.0f - 0.20f * h1);
            if (age > life) continue;
            const float u = age / life;
            const float curve = (u < 0.35f) ? glm::mix(0.3f, 1.0f, u / 0.35f) : glm::mix(1.0f, 0.8f, (u - 0.35f) / 0.65f);
            const glm::vec3 pos = blastCenter + gx_unit_from_index(fi, seedSalt) * (0.20f * h2);
            const float sz = (isBounceOnly ? 0.75f : 1.50f) * glm::mix(0.8f, 1.25f, h1) * curve;
            const float alpha = (1.0f - gx_smoothstep(0.45f, 1.0f, u)) * 0.92f;
            renderer.submit(
                m_fireFlameMesh,
                m_whiteMaterial,
                makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, sz, h2 * GX_TAU + age * 1.5f),
                glm::vec4(evalFireFlameRamp(u * 0.9f, h1), alpha),
                KIND_FIRE_FLAME
            );
        }
    }

    // 4. Burst (amount = 30, lifetime = 0.95 s, spread = 90 deg around normal, speed = 1.5..4.5, gravity_y = +1.8)
    const int burstCount = isBounceOnly ? 5 : 30;
    const float maxBurstLife = isBounceOnly ? 0.35f : 0.95f;
    if (age <= maxBurstLife) {
        for (int i = 0; i < burstCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.33f + seedSalt + 43.0f);
            const float h2 = gx_hash11(fi * 2.67f + seedSalt + 59.0f);
            const float life = maxBurstLife * (1.0f - 0.35f * h1);
            if (age > life) continue;
            const float u = age / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 150.0f, seedSalt + 61.0f);
            const float spd = glm::mix(1.8f, 5.2f, h2) * (isBounceOnly ? 0.65f : 1.0f);
            const float dmp = glm::mix(5.0f, 8.0f, h1);
            glm::vec3 pos = blastCenter + dir * (0.20f * h1 + dampedDistance(spd, dmp, age));
            pos.y += 1.8f * age * age * 0.5f;
            pos.y = (std::max)(pos.y, GROUND_Y + 0.12f);

            const float curve = (u < 0.30f) ? glm::mix(0.35f, 1.0f, u / 0.30f) : glm::mix(1.0f, 1.25f, (u - 0.30f) / 0.70f);
            const float sz = (isBounceOnly ? 0.55f : 1.12f) * glm::mix(0.8f, 1.25f, h1) * curve;
            const float alpha = (1.0f - gx_smoothstep(0.50f, 1.0f, u)) * 0.88f;
            const float angle = h2 * GX_TAU + (h1 - 0.5f) * 3.0f * age;
            renderer.submit(
                m_fireFlameMesh,
                m_whiteMaterial,
                makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, sz, angle),
                glm::vec4(evalFireFlameRamp(u, h2), alpha),
                KIND_FIRE_FLAME
            );
        }
    }

    // 5. Smoke (amount = 14, delay = 0.12 s, lifetime = 1.8 s, explosiveness = 0.3 -> dura hasta ~3.0 s)
    if (!isBounceOnly && age >= 0.12f) {
        const glm::vec3 smokeOrigin = impact.position + normal * 0.60f;
        for (int i = 0; i < 14; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.49f + seedSalt + 83.0f);
            const float h2 = gx_hash11(fi * 2.81f + seedSalt + 97.0f);
            // explosiveness = 0.3 escalona el nacimiento de las nubes de humo hasta ~0.9 s
            const float spawnDelay = 0.12f + (fi / 14.0f) * 0.85f;
            const float pAge = age - spawnDelay;
            const float life = 1.85f * (1.0f - 0.25f * h1);
            if (pAge <= 0.0f || pAge > life) continue;

            const float u = pAge / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 115.0f, seedSalt + 103.0f);
            const float spd = glm::mix(0.7f, 1.8f, h2);
            glm::vec3 pos = smokeOrigin + dir * (0.35f * h1 + dampedDistance(spd, 1.5f, pAge));
            pos.y += 1.05f * pAge;
            pos.y = (std::max)(pos.y, GROUND_Y + 0.20f);

            const float sz = 1.15f * glm::mix(0.8f, 1.25f, h1) * glm::mix(0.4f, 1.9f, u);
            // Gradient_29 de fire_impact.tscn: pico de opacidad 0.60 en u = 0.12..0.60, cae a 0 en u = 1.0
            const float alpha = gx_smoothstep(0.0f, 0.15f, u) * (1.0f - gx_smoothstep(0.55f, 1.0f, u)) * 0.56f;
            const glm::vec3 smokeCol = glm::mix(glm::vec3(0.38f, 0.29f, 0.23f), glm::vec3(0.18f, 0.16f, 0.17f), u);
            renderer.submit(
                m_fireSmokeMesh,
                m_whiteMaterial,
                makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, sz, h2 * GX_TAU + pAge * 0.5f),
                glm::vec4(smokeCol, alpha),
                KIND_FIRE_SMOKE
            );
        }
    }

    // 6. Embers (amount = 44, lifetime = 1.0 s, spread = 80 deg, speed = 3..8, gravity_y = -7.0)
    const int emberCount = isBounceOnly ? 12 : 44;
    const float maxEmberLife = isBounceOnly ? 0.42f : 1.05f;
    if (age <= maxEmberLife) {
        for (int i = 0; i < emberCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.19f + seedSalt + 127.0f);
            const float h2 = gx_hash11(fi * 2.41f + seedSalt + 139.0f);
            const float life = maxEmberLife * (1.0f - 0.45f * h1);
            if (age > life) continue;

            const float u = age / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 155.0f, seedSalt + 149.0f);
            const float spd = glm::mix(3.2f, 8.5f, h2) * (isBounceOnly ? 0.65f : 1.0f);
            const float dmp = glm::mix(1.2f, 2.8f, h1);
            glm::vec3 pos = blastCenter + dir * dampedDistance(spd, dmp, age);
            pos.y -= 0.5f * 7.0f * age * age;
            if (pos.y < GROUND_Y + 0.03f) {
                pos.y = GROUND_Y + 0.03f + std::abs(pos.y - GROUND_Y) * 0.25f;
            }
            const glm::vec3 vel = dir * (spd * std::exp(-dmp * age)) + glm::vec3(0.0f, -7.0f * age, 0.0f);
            const float scale = glm::mix(0.55f, 1.45f, h1) * (1.0f - u);
            const glm::vec3 col = (u < 0.4f)
                ? glm::mix(glm::vec3(1.0f, 0.90f, 0.60f), glm::vec3(1.0f, 0.50f, 0.12f), u / 0.4f)
                : glm::mix(glm::vec3(1.0f, 0.50f, 0.12f), glm::vec3(0.90f, 0.20f, 0.0f), (u - 0.4f) / 0.6f);
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                makeAlignedBillboardMatrix(pos, vel, camPos, camRight, 0.09f * scale, 0.36f * scale),
                glm::vec4(col, 1.0f - u),
                KIND_FIRE_FLASH
            );
        }
    }

    // 7. Luz de explosion (OmniLight3D en fire_impact.tscn: color = (1, 0.5, 0.15), energy = 6.0, range = 7.0, flash = 0.8 s)
    const float flashDur = isBounceOnly ? 0.25f : 0.80f;
    if (age < flashDur) {
        const float lFade = 1.0f - age / flashDur;
        renderer.submitLight({
            impact.position + normal * 0.70f,
            isBounceOnly ? 4.0f : 7.5f,
            FIRE_LIGHT_COLOR,
            (isBounceOnly ? 2.2f : 5.8f) * lFade * lFade
        });
    }
}

void IceLanceVFX::submitLightningCastAndFlight(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    // =========================================================================
    // FASE 1 — LIGHTNING CAST (lightning_cast.tscn, duration = 0.35 s)
    // =========================================================================
    constexpr int CAST_WINDOW_TICKS = 22; // ~0.36 s a 60 Hz
    const int castStartFrame = (std::max)(0, currentFrame - CAST_WINDOW_TICKS);
    for (int t = castStartFrame; t <= currentFrame; ++t) {
        const float castAge = static_cast<float>(currentFrame - t) * CombatSimulation::SECONDS_PER_TICK;
        if (castAge > 0.35f) continue;

        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& e = combatFrames[t][eIdx];
            if (!e.isLightningBolt || e.animationAge < 0.0 || e.animationAge > 0.008)
                continue;

            const glm::vec3 castPos = e.transform.pos;
            const glm::vec3 castDir = directionFromPitchYaw(e.transform.rot);
            const glm::mat3 castBasis = gx_basis_from_normal(castDir);
            const float seedSalt = static_cast<float>(t * 41 + (e.poolIndex >= 0 ? e.poolIndex * 73 : static_cast<int>(eIdx) * 53));

            // 1.1 Flash (lifetime = 0.10 s)
            if (castAge <= 0.10f) {
                const float u = castAge / 0.10f;
                const float curve = (u < 0.3f) ? glm::mix(0.6f, 1.0f, u / 0.3f) : glm::mix(1.0f, 0.3f, (u - 0.3f) / 0.7f);
                const glm::vec3 col = glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.5f, 0.7f, 1.0f), u);
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeBillboardMatrix(castPos, camRight, camUp, camForward, 1.85f * curve),
                    glm::vec4(col, 1.0f - u),
                    KIND_FIRE_FLASH
                );
            }

            // 1.2 Ring (lifetime = 0.20 s)
            if (castAge <= 0.20f) {
                const float u = castAge / 0.20f;
                const float ringRad = glm::mix(0.20f, 0.90f, u);
                const float ringScale = 1.10f;
                glm::mat4 ringModel(1.0f);
                ringModel[0] = glm::vec4(castBasis[0] * ringScale, 0.0f);
                ringModel[1] = glm::vec4(castBasis[1] * ringScale, 0.0f);
                ringModel[2] = glm::vec4(castBasis[2] * ringScale, 0.0f);
                ringModel[3] = glm::vec4(castPos + castDir * 0.12f, 1.0f);
                renderer.submit(
                    m_fireDecalMesh,
                    m_whiteMaterial,
                    ringModel,
                    glm::vec4(ringRad, gx_hash11(seedSalt + 3.0f), 0.85f, (1.0f - u) * 0.90f),
                    KIND_FIRE_DECAL
                );
            }

            // 1.3 6 Arcs radiales (flicker_rate = 24.0, duty = 0.8)
            const float stepId = std::floor(static_cast<float>(timeSeconds) * 24.0f);
            for (int i = 0; i < 6; ++i) {
                const float fi = static_cast<float>(i);
                const float arcSeed = stepId * 0.37f + seedSalt + fi * 13.7f;
                if (gx_hash11(stepId * 1.91f + fi * 7.3f + seedSalt) > 0.80f) continue;
                const glm::vec3 arcDir = gx_unit_from_index(fi, stepId * 0.37f + seedSalt);
                const glm::vec3 arcMid = castPos + arcDir * 0.45f;
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeAlignedBillboardMatrix(arcMid, arcDir, camPos, camRight, 0.55f, 0.90f),
                    glm::vec4(stepId + fi * 11.3f, 0.35f, 0.45f, 1.0f - castAge / 0.35f),
                    KIND_LIGHTNING_BOLT
                );
            }

            // 1.4 22 Sparks (lifetime = 0.35 s, speed = 4..9)
            for (int i = 0; i < 22; ++i) {
                const float fi = static_cast<float>(i);
                const float h1 = gx_hash11(fi * 1.57f + seedSalt + 17.0f);
                const float h2 = gx_hash11(fi * 2.93f + seedSalt + 29.0f);
                const float life = 0.35f * (1.0f - 0.40f * h1);
                if (castAge > life) continue;
                const float u = castAge / life;
                const glm::vec3 dir = sampleDirectionInCone(i, castDir, 80.0f, seedSalt + 31.0f);
                const float spd = glm::mix(4.0f, 9.0f, h2);
                const glm::vec3 pos = castPos + dir * dampedDistance(spd, 4.5f, castAge);
                const glm::vec3 vel = dir * (spd * std::exp(-4.5f * castAge));
                const float sc = glm::mix(0.55f, 1.45f, h1) * (1.0f - u);
                const glm::vec3 col = glm::mix(glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.45f, 0.65f, 1.0f), u);
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeAlignedBillboardMatrix(pos, vel, camPos, camRight, 0.08f * sc, 0.32f * sc),
                    glm::vec4(col, 1.0f - u),
                    KIND_FIRE_FLASH
                );
            }

            // 1.5 Luz de cast (flash = 0.30 s, energy = 3.0)
            if (castAge <= 0.30f) {
                const float lFade = 1.0f - castAge / 0.30f;
                renderer.submitLight({
                    castPos,
                    4.5f,
                    LIGHTNING_LIGHT_COLOR,
                    3.0f * lFade * lFade
                });
            }
        }
    }

    // =========================================================================
    // FASE 2 — LIGHTNING PROJECTILE EN VUELO (lightning_projectile.tscn)
    // =========================================================================
    for (size_t eIdx = 0; eIdx < combatFrames[currentFrame].size(); ++eIdx) {
        const Entidad& e = combatFrames[currentFrame][eIdx];
        if (!e.isLightningBolt)
            continue;

        const glm::vec3 pos = e.transform.pos;
        const float age = static_cast<float>(e.animationAge >= 0.0 ? e.animationAge : timeSeconds);
        const float seedSalt = static_cast<float>(e.poolIndex >= 0 ? e.poolIndex * 59 + 19 : static_cast<int>(eIdx) * 37 + 19);

        // 2.1 Halo electrico azul
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            makeBillboardMatrix(pos, camRight, camUp, camForward, 1.55f),
            glm::vec4(0.40f, 0.58f, 1.0f, 0.42f),
            KIND_FIRE_FLASH
        );

        // 2.2 Nucleo electrico (SphereMesh con KIND_FIRE_ORB)
        Transform orbTransform;
        orbTransform.pos = pos;
        orbTransform.rot = e.transform.rot;
        orbTransform.scale = glm::vec3(0.38f);
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            orbTransform.toMatrix(),
            glm::vec4(0.55f, age * 3.0f, 0.95f, 0.95f),
            KIND_FIRE_ORB
        );

        // 2.3 5 Arcs radiales alrededor del nucleo (flicker_rate = 18.0, duty = 0.55)
        const float stepId = std::floor(static_cast<float>(timeSeconds) * 18.0f);
        for (int i = 0; i < 5; ++i) {
            const float fi = static_cast<float>(i);
            if (gx_hash11(stepId * 1.91f + fi * 7.3f + seedSalt) > 0.55f) continue;
            const glm::vec3 arcDir = gx_unit_from_index(fi, stepId * 0.37f + seedSalt);
            const glm::vec3 arcMid = pos + arcDir * 0.35f;
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                makeAlignedBillboardMatrix(arcMid, arcDir, camPos, camRight, 0.48f, 0.72f),
                glm::vec4(stepId + fi * 13.1f, 0.40f, 0.50f, 0.95f),
                KIND_LIGHTNING_BOLT
            );
        }

        // 2.4 14 Crackle + 40 Sparks en vuelo alrededor del proyectil
        for (int i = 0; i < 14; ++i) {
            const float fi = static_cast<float>(i);
            const float cycle = gx_fract(age / 0.18f + fi / 14.0f);
            const glm::vec3 offsetDir = gx_unit_from_index(fi, seedSalt + std::floor(age / 0.18f + fi / 14.0f));
            const glm::vec3 pPos = pos + offsetDir * (0.12f + 0.32f * cycle);
            const float sz = 0.22f * (1.0f - cycle);
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                makeBillboardMatrix(pPos, camRight, camUp, camForward, sz),
                glm::vec4(0.85f, 0.92f, 1.0f, 1.0f - cycle),
                KIND_FIRE_FLASH
            );
        }

        // 2.5 Luz puntual azul/blanca (energy ~ 2.2)
        const float wave = std::sin(age * 31.0f) * 0.5f + std::sin(age * 11.3f + 2.1f) * 0.5f;
        renderer.submitLight({
            pos,
            5.0f,
            LIGHTNING_LIGHT_COLOR,
            2.2f * (0.85f + 0.15f * wave)
        });
    }

    // =========================================================================
    // 2.6 DOBLE TRAIL ELECTRICO + SPARKS EN ESTELA (0.14 s outer / 0.10 s inner)
    // =========================================================================
    constexpr int LIGHTNING_TRAIL_TICKS = 18; // 0.30 s cubre ambos trails y sparks
    const int trailStartFrame = (std::max)(0, currentFrame - LIGHTNING_TRAIL_TICKS);
    for (int t = trailStartFrame; t <= currentFrame; ++t) {
        const int lagTicks = currentFrame - t;
        const float lagAge = static_cast<float>(lagTicks) * CombatSimulation::SECONDS_PER_TICK;

        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& sample = combatFrames[t][eIdx];
            if (!sample.isLightningBolt)
                continue;

            const glm::vec3 histPos = sample.transform.pos;
            const glm::vec3 histDir = directionFromPitchYaw(sample.transform.rot);
            const float elemIdx = static_cast<float>(t * 23 + (sample.poolIndex >= 0 ? sample.poolIndex * 67 : static_cast<int>(eIdx) * 41));
            const float h1 = gx_hash11(elemIdx * 1.37f + 17.0f);
            const float h2 = gx_hash11(elemIdx * 2.71f + 31.0f);

            // Outer trail: lifetime = 0.14 s, width = 0.60, arco electrico a lo largo de la trayectoria
            if (lagTicks > 0 && lagAge <= 0.14f) {
                const float u = lagAge / 0.14f;
                const float stepId = std::floor(static_cast<float>(timeSeconds) * 20.0f);
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeAlignedBillboardMatrix(histPos, histDir, camPos, camRight, 0.60f * (1.0f - 0.5f * u), 0.72f),
                    glm::vec4(stepId + elemIdx * 0.31f, 0.32f, 0.55f, (1.0f - u) * 0.92f),
                    KIND_LIGHTNING_BOLT
                );
            }

            // Inner trail: lifetime = 0.10 s, width = 0.22, nucleo blanco-azul brillante
            if (lagTicks > 0 && lagAge <= 0.10f) {
                const float u = lagAge / 0.10f;
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeAlignedBillboardMatrix(histPos, histDir, camPos, camRight, 0.22f * (1.0f - 0.4f * u), 0.68f),
                    glm::vec4(0.90f, 0.96f, 1.0f, (1.0f - u) * 0.95f),
                    KIND_FIRE_FLASH
                );
            }

            // Sparks en la estela (lifetime = 0.30 s)
            if (lagAge <= 0.30f) {
                const float u = lagAge / 0.30f;
                const glm::vec3 spread = (gx_hash33(glm::vec3(elemIdx, 19.0f, 7.0f)) - glm::vec3(0.5f)) * 0.75f;
                const glm::vec3 spPos = histPos + spread * (0.2f + u);
                const float sc = 0.14f * (0.55f + 0.90f * h1) * (1.0f - u);
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeBillboardMatrix(spPos, camRight, camUp, camForward, sc),
                    glm::vec4(glm::mix(glm::vec3(1.0f), glm::vec3(0.45f, 0.65f, 1.0f), u), (1.0f - u) * (0.6f + 0.4f * h2)),
                    KIND_FIRE_FLASH
                );
            }
        }
    }
}

void IceLanceVFX::submitLightningImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    // =========================================================================
    // FASE 3 — LIGHTNING IMPACT (lightning_impact.tscn, duration = 2.50 s) y rebote corto
    // =========================================================================
    constexpr float GROUND_Y = 0.0f;
    const glm::vec3 normal = glm::length(impact.normal) > 0.0001f
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::mat3 surfaceBasis = gx_basis_from_normal(normal);
    const float seedSalt = static_cast<float>((impact.startTick * 97) % 997) +
        std::abs(impact.position.x * 17.0f + impact.position.z * 29.0f);
    const float randSeed = gx_hash11(seedSalt + 19.0f);

    // 1. Marcas sobre la superficie orientadas con la normal (Scorch, Burn, Ring)
    if (!isBounceOnly) {
        // Scorch (lifetime = 2.5 s)
        if (age <= 2.5f) {
            const float u = age / 2.5f;
            const float grow = 1.0f - std::pow(1.0f - gx_smoothstep(0.0f, 0.08f, u), 3.0f);
            const float fade = 1.0f - gx_smoothstep(0.70f, 1.0f, u);
            const float scScale = 1.35f * grow;
            glm::mat4 scModel(1.0f);
            scModel[0] = glm::vec4(surfaceBasis[0] * scScale, 0.0f);
            scModel[1] = glm::vec4(surfaceBasis[1] * scScale, 0.0f);
            scModel[2] = glm::vec4(surfaceBasis[2] * scScale, 0.0f);
            scModel[3] = glm::vec4(impact.position + normal * 0.022f, 1.0f);
            renderer.submit(
                m_fireDecalMesh,
                m_whiteMaterial,
                scModel,
                glm::vec4(1.0f - gx_smoothstep(0.0f, 0.35f, age), randSeed, 0.10f, 0.85f * fade),
                KIND_FIRE_DECAL
            );
        }

        // Burn / Glow electrico en superficie (lifetime = 1.0 s)
        if (age <= 1.0f) {
            const float u = age / 1.0f;
            const float grow = 1.0f - std::pow(1.0f - gx_smoothstep(0.0f, 0.10f, u), 3.0f);
            const float fade = 1.0f - gx_smoothstep(0.30f, 1.0f, u);
            const float burnScale = 1.45f * grow;
            glm::mat4 burnModel(1.0f);
            burnModel[0] = glm::vec4(surfaceBasis[0] * burnScale, 0.0f);
            burnModel[1] = glm::vec4(surfaceBasis[1] * burnScale, 0.0f);
            burnModel[2] = glm::vec4(surfaceBasis[2] * burnScale, 0.0f);
            burnModel[3] = glm::vec4(impact.position + normal * 0.030f, 1.0f);
            renderer.submit(
                m_fireDecalMesh,
                m_whiteMaterial,
                burnModel,
                glm::vec4(grow, randSeed, 0.45f, fade * 0.75f),
                KIND_FIRE_DECAL
            );
        }
    }

    // Ring (lifetime = 0.35 s en impacto, 0.20 s en rebote)
    const float ringLife = isBounceOnly ? 0.20f : 0.35f;
    if (age <= ringLife) {
        const float u = age / ringLife;
        const float grow = 1.0f - std::pow(1.0f - gx_smoothstep(0.0f, 0.80f, u), 3.0f);
        const float ringRad = glm::mix(0.12f, 0.92f, grow);
        const float fade = 1.0f - gx_smoothstep(0.35f, 1.0f, u);
        const float ringScale = isBounceOnly ? 0.90f : 1.85f;
        glm::mat4 ringModel(1.0f);
        ringModel[0] = glm::vec4(surfaceBasis[0] * ringScale, 0.0f);
        ringModel[1] = glm::vec4(surfaceBasis[1] * ringScale, 0.0f);
        ringModel[2] = glm::vec4(surfaceBasis[2] * ringScale, 0.0f);
        ringModel[3] = glm::vec4(impact.position + normal * 0.038f, 1.0f);
        renderer.submit(
            m_fireDecalMesh,
            m_whiteMaterial,
            ringModel,
            glm::vec4(ringRad, randSeed, 0.85f, fade),
            KIND_FIRE_DECAL
        );
    }

    const glm::vec3 blastCenter = impact.position + normal * (isBounceOnly ? 0.20f : 0.45f);

    // 2. Flash inicial (lifetime = 0.12 s)
    if (age <= 0.12f) {
        const float u = age / 0.12f;
        const float curve = (u < 0.25f) ? glm::mix(0.5f, 1.0f, u / 0.25f) : glm::mix(1.0f, 0.3f, (u - 0.25f) / 0.75f);
        const float flashSize = (isBounceOnly ? 1.3f : 2.9f) * curve;
        const glm::vec3 col = glm::mix(glm::vec3(1.0f), glm::vec3(0.5f, 0.65f, 1.0f), u);
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            makeBillboardMatrix(blastCenter, camRight, camUp, camForward, flashSize),
            glm::vec4(col, 1.0f - u),
            KIND_FIRE_FLASH
        );
    }

    // 3. Shock sphere (SphereMesh shell expandiendose durante 0.35 s)
    if (!isBounceOnly && age <= 0.35f) {
        const float u = age / 0.35f;
        const float sc = glm::mix(0.2f, 1.3f, u) * 0.85f;
        Transform shockT;
        shockT.pos = blastCenter;
        shockT.scale = glm::vec3(sc);
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            shockT.toMatrix(),
            glm::vec4(0.55f, age * 3.0f, 0.95f, (1.0f - gx_smoothstep(0.4f, 1.0f, u)) * 0.75f),
            KIND_FIRE_ORB
        );
    }

    // 4. Arcs radiales (8 en impacto completo, 3 en rebote)
    const int arcCount = isBounceOnly ? 3 : 8;
    const float maxArcLife = isBounceOnly ? 0.30f : 0.65f;
    if (age <= maxArcLife) {
        const float stepId = std::floor(static_cast<float>(timeSeconds) * 16.0f);
        const float fade = 1.0f - gx_smoothstep(0.5f, 1.0f, age / maxArcLife);
        for (int i = 0; i < arcCount; ++i) {
            const float fi = static_cast<float>(i);
            if (gx_hash11(stepId * 1.91f + fi * 7.3f + seedSalt) > 0.75f) continue;
            const glm::vec3 arcDir = sampleDirectionInCone(i, normal, 150.0f, seedSalt + stepId * 0.37f);
            const float len = isBounceOnly ? 0.70f : 1.35f;
            const glm::vec3 arcMid = blastCenter + arcDir * (len * 0.5f);
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                makeAlignedBillboardMatrix(arcMid, arcDir, camPos, camRight, 0.55f, len),
                glm::vec4(stepId + fi * 13.7f, 0.30f, 0.50f, fade),
                KIND_LIGHTNING_BOLT
            );
        }
    }

    // 5. Sparks (46 en impacto completo, 10 en rebote)
    const int sparkCount = isBounceOnly ? 10 : 46;
    const float maxSparkLife = isBounceOnly ? 0.35f : 0.85f;
    if (age <= maxSparkLife) {
        for (int i = 0; i < sparkCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.23f + seedSalt + 113.0f);
            const float h2 = gx_hash11(fi * 2.51f + seedSalt + 127.0f);
            const float life = maxSparkLife * (1.0f - 0.45f * h1);
            if (age > life) continue;
            const float u = age / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 155.0f, seedSalt + 131.0f);
            const float spd = glm::mix(3.5f, 9.0f, h2) * (isBounceOnly ? 0.65f : 1.0f);
            glm::vec3 pos = blastCenter + dir * dampedDistance(spd, 2.5f, age);
            pos.y -= 0.5f * 4.0f * age * age;
            pos.y = (std::max)(pos.y, GROUND_Y + 0.03f);
            const glm::vec3 vel = dir * (spd * std::exp(-2.5f * age)) + glm::vec3(0.0f, -4.0f * age, 0.0f);
            const float sc = glm::mix(0.55f, 1.45f, h1) * (1.0f - u);
            const glm::vec3 col = glm::mix(glm::vec3(1.0f), glm::vec3(0.45f, 0.65f, 1.0f), u);
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                makeAlignedBillboardMatrix(pos, vel, camPos, camRight, 0.09f * sc, 0.36f * sc),
                glm::vec4(col, 1.0f - u),
                KIND_FIRE_FLASH
            );
        }
    }

    // 6. Luz intensa de impacto (energy ~ 7.0)
    const float flashDur = isBounceOnly ? 0.22f : 0.60f;
    if (age < flashDur) {
        const float lFade = 1.0f - age / flashDur;
        renderer.submitLight({
            impact.position + normal * 0.70f,
            isBounceOnly ? 4.0f : 7.5f,
            LIGHTNING_LIGHT_COLOR,
            (isBounceOnly ? 2.4f : 7.0f) * lFade * lFade
        });
    }
}

void IceLanceVFX::submitEarthCastAndFlight(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    // =========================================================================
    // FASE 1 — EARTH CAST (0.30 s, reutilizando polvo, piedrecitas y flash)
    // =========================================================================
    constexpr int CAST_WINDOW_TICKS = 19; // ~0.31 s a 60 Hz
    const int castStartFrame = (std::max)(0, currentFrame - CAST_WINDOW_TICKS);
    for (int t = castStartFrame; t <= currentFrame; ++t) {
        const float castAge = static_cast<float>(currentFrame - t) * CombatSimulation::SECONDS_PER_TICK;
        if (castAge > 0.30f) continue;

        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& e = combatFrames[t][eIdx];
            if (!e.isEarthSpike || e.animationAge < 0.0 || e.animationAge > 0.008)
                continue;

            const glm::vec3 castPos = e.transform.pos;
            const glm::vec3 castDir = directionFromPitchYaw(e.transform.rot);
            const float seedSalt = static_cast<float>(t * 37 + (e.poolIndex >= 0 ? e.poolIndex * 71 : static_cast<int>(eIdx) * 47));

            // Flash marron/naranja corto (0.12 s)
            if (castAge <= 0.12f) {
                const float u = castAge / 0.12f;
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeBillboardMatrix(castPos, camRight, camUp, camForward, 1.45f * (1.0f - 0.5f * u)),
                    glm::vec4(1.0f, 0.69f, 0.38f, 1.0f - u),
                    KIND_FIRE_FLASH
                );
            }

            // Nube de polvo (4 puffs)
            for (int i = 0; i < 4; ++i) {
                const float fi = static_cast<float>(i);
                const float h1 = gx_hash11(fi * 1.43f + seedSalt);
                const float u = castAge / 0.30f;
                const glm::vec3 dir = sampleDirectionInCone(i, castDir, 65.0f, seedSalt + 7.0f);
                const glm::vec3 pos = castPos + dir * (0.4f * castAge) + glm::vec3(0.0f, 0.3f * castAge, 0.0f);
                renderer.submit(
                    m_fireSmokeMesh,
                    m_whiteMaterial,
                    makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, 0.75f * (0.5f + u), h1 * GX_TAU),
                    glm::vec4(0.40f, 0.33f, 0.27f, (1.0f - u) * 0.55f),
                    KIND_FIRE_SMOKE
                );
            }

            // 5 piedrecitas
            for (int i = 0; i < 5; ++i) {
                const float fi = static_cast<float>(i);
                const float h1 = gx_hash11(fi * 1.71f + seedSalt + 13.0f);
                const float u = castAge / 0.30f;
                const glm::vec3 dir = sampleDirectionInCone(i, castDir, 70.0f, seedSalt + 19.0f);
                glm::vec3 pos = castPos + dir * (2.6f * castAge);
                pos.y -= 0.5f * 6.0f * castAge * castAge;
                Transform pT;
                pT.pos = pos;
                pT.rot = glm::vec3(castAge * 6.0f, h1 * GX_TAU, castAge * 5.0f);
                pT.scale = glm::vec3(0.11f * (1.0f - u));
                renderer.submit(m_earthRockMesh, m_whiteMaterial, pT.toMatrix(), glm::vec4(0.45f, 0.37f, 0.30f, 1.0f), 0);
            }
        }
    }

    // =========================================================================
    // FASE 2 — EARTH PROJECTILE EN VUELO (rock_throw.tscn)
    // =========================================================================
    for (size_t eIdx = 0; eIdx < combatFrames[currentFrame].size(); ++eIdx) {
        const Entidad& e = combatFrames[currentFrame][eIdx];
        if (!e.isEarthSpike)
            continue;

        const glm::vec3 pos = e.transform.pos;
        const float age = static_cast<float>(e.animationAge >= 0.0 ? e.animationAge : timeSeconds);

        // Roca principal (rock.obj) con rotacion angular_velocity = 360 deg/s
        Transform rockT;
        rockT.pos = pos;
        rockT.rot = e.transform.rot;
        rockT.rot.z = std::fmod(age * GX_TAU, GX_TAU);
        rockT.rot.x += std::fmod(age * 2.5f, GX_TAU);
        rockT.scale = glm::vec3(0.68f);
        renderer.submit(
            m_earthRockMesh,
            m_whiteMaterial,
            rockT.toMatrix(),
            glm::vec4(0.42f, 0.34f, 0.28f, 1.0f),
            0
        );

        // Luz calida sutil acompaniando a la roca
        renderer.submitLight({
            pos,
            3.5f,
            EARTH_LIGHT_COLOR,
            1.1f
        });
    }

    // =========================================================================
    // 2.5 TRAIL DE ROCK THROW (Trail polvo, Pebbles, Streak)
    // =========================================================================
    constexpr int EARTH_TRAIL_TICKS = 36; // 0.60 s a 60 Hz
    const int trailStartFrame = (std::max)(0, currentFrame - EARTH_TRAIL_TICKS);
    for (int t = trailStartFrame; t <= currentFrame; ++t) {
        const int lagTicks = currentFrame - t;
        const float lagAge = static_cast<float>(lagTicks) * CombatSimulation::SECONDS_PER_TICK;

        for (size_t eIdx = 0; eIdx < combatFrames[t].size(); ++eIdx) {
            const Entidad& sample = combatFrames[t][eIdx];
            if (!sample.isEarthSpike)
                continue;

            const glm::vec3 histPos = sample.transform.pos;
            const glm::vec3 histDir = directionFromPitchYaw(sample.transform.rot);
            const float elemIdx = static_cast<float>(t * 29 + (sample.poolIndex >= 0 ? sample.poolIndex * 73 : static_cast<int>(eIdx) * 43));
            const float h1 = gx_hash11(elemIdx * 1.33f + 11.0f);
            const float h2 = gx_hash11(elemIdx * 2.57f + 23.0f);

            // A) Trail de polvo marron/gris (0.60 s)
            if (t % 2 == 0 && lagAge <= 0.60f) {
                const float u = lagAge / 0.60f;
                const glm::vec3 randOff = (gx_hash33(glm::vec3(elemIdx, 13.0f, 5.0f)) - glm::vec3(0.5f)) * 0.35f;
                glm::vec3 dustPos = histPos + randOff * (0.25f + u);
                dustPos.y += 0.4f * lagAge;
                const float sz = 0.65f * glm::mix(0.4f, 1.0f, gx_smoothstep(0.0f, 0.3f, u)) * (0.75f + 0.5f * h1);
                const float alpha = (1.0f - gx_smoothstep(0.35f, 1.0f, u)) * 0.52f;
                const glm::vec3 dustCol = glm::mix(glm::vec3(0.37f, 0.31f, 0.27f), glm::vec3(0.23f, 0.19f, 0.17f), u);
                renderer.submit(
                    m_fireSmokeMesh,
                    m_whiteMaterial,
                    makeRotatedBillboardMatrix(dustPos, camRight, camUp, camForward, sz, h2 * GX_TAU + lagAge * 0.7f),
                    glm::vec4(dustCol, alpha),
                    KIND_FIRE_SMOKE
                );
            }

            // B) Pebbles (piedritas que salen alrededor y caen ligeramente)
            if (t % 3 == 0 && lagAge <= 0.45f) {
                const float u = lagAge / 0.45f;
                const glm::vec3 spread = (gx_hash33(glm::vec3(elemIdx, 31.0f, 9.0f)) - glm::vec3(0.5f)) * 1.4f;
                glm::vec3 pebPos = histPos + spread * lagAge;
                pebPos.y -= 0.5f * 5.0f * lagAge * lagAge;
                Transform pebT;
                pebT.pos = pebPos;
                pebT.rot = glm::vec3(lagAge * 7.0f, h1 * GX_TAU, lagAge * 5.0f);
                pebT.scale = glm::vec3(0.11f * (1.0f - 0.7f * u));
                renderer.submit(m_earthRockMesh, m_whiteMaterial, pebT.toMatrix(), glm::vec4(0.48f, 0.40f, 0.32f, 1.0f), 0);
            }

            // C) Streak (lineas rapidas de movimiento detras del proyectil, 0.18 s)
            if (lagTicks > 0 && lagAge <= 0.18f) {
                const float u = lagAge / 0.18f;
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    makeAlignedBillboardMatrix(histPos, histDir, camPos, camRight, 0.24f * (1.0f - u), 0.75f),
                    glm::vec4(0.85f, 0.72f, 0.55f, (1.0f - u) * 0.65f),
                    KIND_FIRE_FLASH
                );
            }
        }
    }
}

void IceLanceVFX::submitEarthImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward) const
{
    // =========================================================================
    // FASE 3 — EARTH IMPACT (rock_hit.tscn, duration = 1.60 s) y rebote corto
    // =========================================================================
    constexpr float GROUND_Y = 0.0f;
    const glm::vec3 normal = glm::length(impact.normal) > 0.0001f
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);
    const float seedSalt = static_cast<float>((impact.startTick * 83) % 997) +
        std::abs(impact.position.x * 13.0f + impact.position.z * 23.0f);
    const glm::vec3 blastCenter = impact.position + normal * (isBounceOnly ? 0.18f : 0.35f);

    // 1. Flash inicial corto (0.14 s)
    if (age <= 0.14f) {
        const float u = age / 0.14f;
        const float curve = (u < 0.3f) ? glm::mix(0.4f, 1.0f, u / 0.3f) : glm::mix(1.0f, 0.8f, (u - 0.3f) / 0.7f);
        const glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.96f, 0.86f), glm::vec3(1.0f, 0.70f, 0.28f), u);
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            makeBillboardMatrix(blastCenter, camRight, camUp, camForward, (isBounceOnly ? 1.1f : 2.1f) * curve),
            glm::vec4(col, 1.0f - u),
            KIND_FIRE_FLASH
        );
    }

    // 2. Burst (0.32 s)
    if (!isBounceOnly && age <= 0.32f) {
        const float u = age / 0.32f;
        const glm::vec3 col = glm::mix(glm::vec3(1.0f, 0.88f, 0.62f), glm::vec3(0.72f, 0.31f, 0.10f), u);
        renderer.submit(
            m_fireFlameMesh,
            m_whiteMaterial,
            makeBillboardMatrix(blastCenter, camRight, camUp, camForward, 1.85f * glm::mix(0.3f, 1.1f, u)),
            glm::vec4(col, (1.0f - u) * 0.85f),
            KIND_FIRE_FLAME
        );
    }

    // 3. Chips (fragmentos de roca low-poly orientados con la normal, con gravedad y rebote)
    const int chipCount = isBounceOnly ? 4 : 20;
    const float maxChipLife = isBounceOnly ? 0.38f : 1.10f;
    if (age <= maxChipLife) {
        for (int i = 0; i < chipCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.29f + seedSalt + 17.0f);
            const float h2 = gx_hash11(fi * 2.53f + seedSalt + 29.0f);
            const float life = maxChipLife * (0.70f + 0.30f * h1);
            if (age > life) continue;

            const float u = age / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 130.0f, seedSalt + 41.0f);
            const float spd = glm::mix(3.0f, 7.5f, h2) * (isBounceOnly ? 0.6f : 1.0f);
            glm::vec3 pos = blastCenter + dir * (spd * age);
            pos.y -= 0.5f * 9.5f * age * age;
            if (pos.y < GROUND_Y + 0.05f) {
                pos.y = GROUND_Y + 0.05f + std::abs(pos.y - GROUND_Y) * 0.25f;
            }
            Transform chipT;
            chipT.pos = pos;
            chipT.rot = glm::vec3(age * 8.0f * (0.5f + h1), h2 * GX_TAU, age * 6.0f);
            chipT.scale = glm::vec3((isBounceOnly ? 0.14f : 0.22f) * (0.6f + 0.6f * h2) * (1.0f - gx_smoothstep(0.7f, 1.0f, u)));
            renderer.submit(m_earthRockMesh, m_whiteMaterial, chipT.toMatrix(), glm::vec4(0.45f, 0.36f, 0.29f, 1.0f), 0);
        }
    }

    // 4. Dust (polvo marron/gris que se expande y asciende)
    const int dustCount = isBounceOnly ? 3 : 12;
    const float maxDustLife = isBounceOnly ? 0.38f : 1.45f;
    if (age <= maxDustLife) {
        for (int i = 0; i < dustCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.47f + seedSalt + 61.0f);
            const float h2 = gx_hash11(fi * 2.71f + seedSalt + 73.0f);
            const float life = maxDustLife * (0.70f + 0.30f * h1);
            if (age > life) continue;

            const float u = age / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 120.0f, seedSalt + 83.0f);
            glm::vec3 pos = blastCenter + dir * dampedDistance(glm::mix(1.2f, 2.8f, h2), 2.5f, age);
            pos.y += 0.6f * age;
            pos.y = (std::max)(pos.y, GROUND_Y + 0.15f);
            const float sz = (isBounceOnly ? 0.7f : 1.35f) * (0.8f + 0.45f * h1) * glm::mix(0.4f, 1.25f, u);
            const float alpha = gx_smoothstep(0.0f, 0.15f, u) * (1.0f - gx_smoothstep(0.45f, 1.0f, u)) * 0.58f;
            const glm::vec3 dustCol = glm::mix(glm::vec3(0.42f, 0.35f, 0.29f), glm::vec3(0.24f, 0.20f, 0.18f), u);
            renderer.submit(
                m_fireSmokeMesh,
                m_whiteMaterial,
                makeRotatedBillboardMatrix(pos, camRight, camUp, camForward, sz, h2 * GX_TAU + age * 0.5f),
                glm::vec4(dustCol, alpha),
                KIND_FIRE_SMOKE
            );
        }
    }

    // 5. Sparks (pequenas chispas calidas)
    const int sparkCount = isBounceOnly ? 4 : 18;
    const float maxSparkLife = isBounceOnly ? 0.30f : 0.70f;
    if (age <= maxSparkLife) {
        for (int i = 0; i < sparkCount; ++i) {
            const float fi = static_cast<float>(i);
            const float h1 = gx_hash11(fi * 1.19f + seedSalt + 101.0f);
            const float h2 = gx_hash11(fi * 2.41f + seedSalt + 113.0f);
            const float life = maxSparkLife * (0.65f + 0.35f * h1);
            if (age > life) continue;

            const float u = age / life;
            const glm::vec3 dir = sampleDirectionInCone(i, normal, 145.0f, seedSalt + 127.0f);
            const float spd = glm::mix(3.0f, 7.0f, h2);
            glm::vec3 pos = blastCenter + dir * dampedDistance(spd, 3.0f, age);
            pos.y -= 0.5f * 6.0f * age * age;
            const glm::vec3 vel = dir * (spd * std::exp(-3.0f * age)) + glm::vec3(0.0f, -6.0f * age, 0.0f);
            const float sc = (0.6f + 0.8f * h1) * (1.0f - u);
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                makeAlignedBillboardMatrix(pos, vel, camPos, camRight, 0.07f * sc, 0.28f * sc),
                glm::vec4(1.0f, 0.75f, 0.35f, 1.0f - u),
                KIND_FIRE_FLASH
            );
        }
    }

    // 6. Luz breve (OmniLight3D)
    const float flashDur = isBounceOnly ? 0.18f : 0.45f;
    if (age < flashDur) {
        const float lFade = 1.0f - age / flashDur;
        renderer.submitLight({
            impact.position + normal * 0.55f,
            isBounceOnly ? 3.5f : 6.0f,
            EARTH_LIGHT_COLOR,
            (isBounceOnly ? 1.8f : 4.2f) * lFade * lFade
        });
    }
}

void IceLanceVFX::submitWindCastAndFlight(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    if (currentFrame < 0 || currentFrame >= static_cast<int>(combatFrames.size())) return;
    if (!m_fireOrbMesh || !m_fireFlashMesh || !m_fireDecalMesh || !m_fireSmokeMesh || !m_whiteTexture) return;

    const auto& frameEntities = combatFrames[currentFrame];

    for (size_t eIdx = 0; eIdx < frameEntities.size(); ++eIdx) {
        const Entidad& e = frameEntities[eIdx];
        if (!e.isWindBlade) continue;

        const glm::vec3 pos = e.transform.pos;
        const glm::vec3 vel = directionFromPitchYaw(e.transform.rot);
        const float baseScale = (std::max)(0.55f, e.transform.scale.x * 0.50f);
        const float t = static_cast<float>(timeSeconds);
        const float seed = static_cast<float>(e.poolIndex >= 0 ? e.poolIndex * 73 : static_cast<int>(eIdx) * 43) * 0.37f;

        // ------------------------------------------------------------------
        // PHASE 1: CAST (primeros instantes del proyectil)
        // ------------------------------------------------------------------
        if (e.animationAge <= 0.008f) {
            const float castProgress = gx_clamp(e.animationAge / 0.008f, 0.0f, 1.0f);
            const float castFade = 1.0f - castProgress;

            // Anillo/remolino de aire comprimido en el punto de cast
            const glm::mat4 castSwirlMat = glm::rotate(
                makeAxisAlignedMatrix(pos, vel, baseScale * (0.85f + castProgress * 1.15f)),
                t * 10.0f + seed,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            renderer.submit(
                m_fireDecalMesh,
                m_whiteMaterial,
                castSwirlMat,
                glm::vec4(0.82f, 0.96f, 0.92f, 0.72f * castFade),
                KIND_WIND_DECAL
            );

            // Líneas curvas de viento convergiendo
            const glm::vec3 upHint = (std::abs(vel.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::vec3 right = glm::normalize(glm::cross(upHint, vel));
            const glm::vec3 up = glm::normalize(glm::cross(vel, right));
            for (int k = 0; k < 4; ++k) {
                const float angle = seed + static_cast<float>(k) * 1.5707963f + t * 8.0f;
                const glm::vec3 radial = std::cos(angle) * right + std::sin(angle) * up;
                const glm::vec3 streakPos = pos + radial * (baseScale * (0.9f - castProgress * 0.5f));
                const glm::mat4 streakMat = makeRotatedBillboardMatrix(
                    streakPos, camRight, camUp, camForward, baseScale * 0.75f, angle
                );
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    streakMat,
                    glm::vec4(0.88f, 0.98f, 0.95f, 0.75f * castFade),
                    KIND_WIND_STREAK
                );
            }
        }

        // ------------------------------------------------------------------
        // PHASE 2: PROJECTILE (Orbe de aire comprimido + corrientes en espiral)
        // ------------------------------------------------------------------
        // 2.1 Orbe interior de aire comprimido (Himmelblau wind preset: tint dff7ee / body 9ab0a8)
        glm::mat4 coreOrbMat = glm::translate(glm::mat4(1.0f), pos);
        coreOrbMat = glm::rotate(coreOrbMat, t * 5.2f + seed, vel);
        coreOrbMat = glm::scale(coreOrbMat, glm::vec3(baseScale * 0.68f));
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            coreOrbMat,
            glm::vec4(0.87f, 0.97f, 0.93f, 0.88f),
            KIND_WIND_ORB
        );

        // 2.2 Capa exterior translúcida de aire giratorio
        glm::mat4 outerOrbMat = glm::translate(glm::mat4(1.0f), pos);
        outerOrbMat = glm::rotate(outerOrbMat, -t * 7.5f + seed, glm::normalize(vel + glm::vec3(0.2f, 0.3f, 0.1f)));
        outerOrbMat = glm::scale(outerOrbMat, glm::vec3(baseScale * 1.02f));
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            outerOrbMat,
            glm::vec4(0.95f, 1.00f, 0.98f, 0.42f),
            KIND_WIND_ORB
        );

        // 2.3 Corrientes curvas / espirales rodeando el orbe
        for (int s = 0; s < 3; ++s) {
            const float phase = t * 6.5f + seed + static_cast<float>(s) * 2.094395f;
            const glm::vec3 offsetDir = glm::normalize(glm::vec3(std::cos(phase), std::sin(phase * 1.3f), std::sin(phase)));
            const glm::vec3 streakPos = pos + offsetDir * (baseScale * 0.42f);
            const glm::mat4 streakMat = makeRotatedBillboardMatrix(
                streakPos, camRight, camUp, camForward, baseScale * 0.95f, phase
            );
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                streakMat,
                glm::vec4(0.90f, 0.98f, 0.95f, 0.72f),
                KIND_WIND_STREAK
            );
        }

        // ------------------------------------------------------------------
        // PHASE 2.4: TRAIL (Estela de remolinos y partículas de aire desde combatFrames)
        // ------------------------------------------------------------------
        constexpr int WIND_TRAIL_TICKS = 16;
        const int trailStart = (std::max)(0, currentFrame - WIND_TRAIL_TICKS);
        for (int f = currentFrame - 1; f >= trailStart; --f) {
            const int back = currentFrame - f;
            const float ratio = 1.0f - static_cast<float>(back) / static_cast<float>(WIND_TRAIL_TICKS + 1);
            for (size_t pIdx = 0; pIdx < combatFrames[f].size(); ++pIdx) {
                const Entidad& prevE = combatFrames[f][pIdx];
                if (!prevE.isWindBlade) continue;
                if (e.poolIndex >= 0 && prevE.poolIndex != e.poolIndex) continue;

                const float trailScale = baseScale * (0.80f * ratio + 0.15f);
                if (back % 2 == 1) {
                    const glm::mat4 trailStreakMat = makeRotatedBillboardMatrix(
                        prevE.transform.pos,
                        camRight,
                        camUp,
                        camForward,
                        trailScale * 1.15f,
                        t * 7.0f + static_cast<float>(back) * 0.85f + seed
                    );
                    renderer.submit(
                        m_fireFlashMesh,
                        m_whiteMaterial,
                        trailStreakMat,
                        glm::vec4(0.84f, 0.96f, 0.92f, 0.44f * ratio),
                        KIND_WIND_STREAK
                    );
                }

                if (back % 3 == 0) {
                    const glm::mat4 mistMat = makeRotatedBillboardMatrix(
                        prevE.transform.pos,
                        camRight,
                        camUp,
                        camForward,
                        trailScale * 0.85f,
                        seed + static_cast<float>(back) * 0.5f
                    );
                    renderer.submit(
                        m_fireSmokeMesh,
                        m_whiteMaterial,
                        mistMat,
                        glm::vec4(0.86f, 0.95f, 0.93f, 0.28f * ratio),
                        KIND_FIRE_SMOKE
                    );
                }
                break;
            }
        }

        // 2.5 Luz sutil del proyectil
        renderer.submitLight({
            pos,
            baseScale * 3.8f,
            WIND_LIGHT_COLOR,
            1.65f
        });
    }
}

void IceLanceVFX::submitWindImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward) const
{
    if (!m_fireOrbMesh || !m_fireFlashMesh || !m_fireDecalMesh || !m_fireSmokeMesh || !m_whiteTexture) return;
    (void)camPos;

    const float duration = isBounceOnly ? 0.40f : 1.20f;
    const float progress = gx_clamp(age / duration, 0.0f, 1.0f);
    if (progress >= 1.0f) return;

    const glm::vec3 normal = (glm::length(impact.normal) > 0.001f)
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const float scaleMult = isBounceOnly ? 0.58f : 1.0f;
    const float alphaMult = isBounceOnly ? 0.65f : 1.0f;
    const float seed = gx_hash11(impact.position.x * 13.7f + impact.position.z * 7.3f) * 100.0f;

    // 1. Onda expansiva / remolino superficial orientado por normal (Himmelblau wind-hit)
    const float ringExpand = 0.45f + gx_smoothstep(0.0f, 0.65f, progress) * 2.45f;
    const float ringFade = 1.0f - gx_smoothstep(0.15f, 1.0f, progress);
    const glm::mat4 decalMat = glm::rotate(
        makeAxisAlignedMatrix(impact.position + normal * 0.04f, normal, 1.35f * scaleMult * ringExpand),
        age * 6.5f + seed,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    renderer.submit(
        m_fireDecalMesh,
        m_whiteMaterial,
        decalMat,
        glm::vec4(0.86f, 0.97f, 0.93f, 0.85f * ringFade * alphaMult),
        KIND_WIND_DECAL
    );

    // 2. Esfera de aire comprimido que se expande y disipa rápidamente
    if (progress < 0.55f) {
        const float shellP = progress / 0.55f;
        const float shellScale = (0.45f + shellP * 1.85f) * scaleMult;
        const float shellFade = (1.0f - shellP) * (1.0f - shellP);
        glm::mat4 shellMat = glm::translate(glm::mat4(1.0f), impact.position + normal * 0.25f);
        shellMat = glm::rotate(shellMat, age * 8.0f + seed, normal);
        shellMat = glm::scale(shellMat, glm::vec3(shellScale));
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            shellMat,
            glm::vec4(0.90f, 0.98f, 0.95f, 0.72f * shellFade * alphaMult),
            KIND_WIND_ORB
        );
    }

    const glm::vec3 upHint = (std::abs(normal.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(upHint, normal));
    const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

    // 3. Ráfagas curvas de viento saliendo hacia afuera
    const int streakCount = isBounceOnly ? 4 : 8;
    for (int i = 0; i < streakCount; ++i) {
        const float angle = seed + static_cast<float>(i) * (GX_TAU / static_cast<float>(streakCount));
        const glm::vec3 dir = glm::normalize(normal * 0.45f + (std::cos(angle) * tangent + std::sin(angle) * bitangent) * 0.9f);
        const float dist = dampedDistance(age, isBounceOnly ? 4.2f : 6.8f, 3.6f) * scaleMult;
        const glm::vec3 pos = impact.position + normal * 0.15f + dir * dist;
        const float streakFade = 1.0f - gx_smoothstep(0.08f, 0.75f, progress);
        if (streakFade <= 0.01f) continue;

        const glm::mat4 streakMat = makeRotatedBillboardMatrix(
            pos,
            camRight,
            camUp,
            camForward,
            (0.65f + progress * 0.75f) * scaleMult,
            static_cast<float>(i) * 0.9f + age * 5.0f
        );
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            streakMat,
            glm::vec4(0.88f, 0.98f, 0.95f, 0.80f * streakFade * alphaMult),
            KIND_WIND_STREAK
        );
    }

    // 4. Partículas de polvo / aire disipándose
    const int dustCount = isBounceOnly ? 3 : 6;
    for (int i = 0; i < dustCount; ++i) {
        const float angle = seed + 0.5f + static_cast<float>(i) * (GX_TAU / static_cast<float>(dustCount));
        const glm::vec3 dir = glm::normalize(normal * 0.35f + (std::cos(angle) * tangent + std::sin(angle) * bitangent) * 0.95f);
        const float dist = dampedDistance(age, isBounceOnly ? 2.5f : 4.2f, 2.8f) * scaleMult;
        const glm::vec3 pos = impact.position + normal * 0.18f + dir * dist;
        const float dustFade = 1.0f - gx_smoothstep(0.20f, 1.0f, progress);
        if (dustFade <= 0.01f) continue;

        const glm::mat4 dustMat = makeRotatedBillboardMatrix(
            pos,
            camRight,
            camUp,
            camForward,
            (0.55f + progress * 0.85f) * scaleMult,
            seed + static_cast<float>(i)
        );
        renderer.submit(
            m_fireSmokeMesh,
            m_whiteMaterial,
            dustMat,
            glm::vec4(0.86f, 0.95f, 0.93f, 0.36f * dustFade * alphaMult),
            KIND_FIRE_SMOKE
        );
    }

    // 5. Luz breve de impacto
    const float flashDur = isBounceOnly ? 0.14f : 0.32f;
    if (age < flashDur) {
        const float lFade = 1.0f - age / flashDur;
        renderer.submitLight({
            impact.position + normal * 0.45f,
            isBounceOnly ? 3.2f : 5.5f,
            WIND_LIGHT_COLOR,
            (isBounceOnly ? 1.4f : 3.2f) * lFade * lFade
        });
    }
}

void IceLanceVFX::submitWaterCastAndFlight(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    if (currentFrame < 0 || currentFrame >= static_cast<int>(combatFrames.size())) return;
    if (!m_fireOrbMesh || !m_fireFlashMesh || !m_fireDecalMesh || !m_fireSmokeMesh || !m_whiteTexture) return;
    (void)camPos;

    const glm::vec3 WATER_LIGHT_COLOR(0.32f, 0.78f, 0.96f);
    const auto& frameEntities = combatFrames[currentFrame];

    for (size_t eIdx = 0; eIdx < frameEntities.size(); ++eIdx) {
        const Entidad& e = frameEntities[eIdx];
        if (!e.isWaterBolt) continue;

        const glm::vec3 pos = e.transform.pos;
        const glm::vec3 vel = directionFromPitchYaw(e.transform.rot);
        const float baseScale = (std::max)(0.52f, e.transform.scale.x * 0.52f);
        const float t = static_cast<float>(timeSeconds);
        const float seed = static_cast<float>(e.poolIndex >= 0 ? e.poolIndex * 73 : static_cast<int>(eIdx) * 43) * 0.37f;

        // ------------------------------------------------------------------
        // PHASE 1: CAST (concentración de agua + anillo/remolino + gotas)
        // ------------------------------------------------------------------
        if (e.animationAge <= 0.008f) {
            const float castProgress = gx_clamp(e.animationAge / 0.008f, 0.0f, 1.0f);
            const float castFade = 1.0f - castProgress;

            const glm::mat4 castRingMat = glm::rotate(
                makeAxisAlignedMatrix(pos, vel, baseScale * (0.8f + castProgress * 1.1f)),
                t * 9.0f + seed,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            renderer.submit(
                m_fireDecalMesh,
                m_whiteMaterial,
                castRingMat,
                glm::vec4(0.55f, 0.88f, 0.98f, 0.68f * castFade),
                KIND_WATER_MIST
            );

            const glm::vec3 upHint = (std::abs(vel.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
            const glm::vec3 right = glm::normalize(glm::cross(upHint, vel));
            const glm::vec3 up = glm::normalize(glm::cross(vel, right));
            for (int k = 0; k < 5; ++k) {
                const float angle = seed + static_cast<float>(k) * (GX_TAU / 5.0f) + t * 7.0f;
                const glm::vec3 radial = std::cos(angle) * right + std::sin(angle) * up;
                const glm::vec3 dropPos = pos + radial * (baseScale * (0.85f - castProgress * 0.45f));
                const glm::mat4 dropMat = makeRotatedBillboardMatrix(
                    dropPos, camRight, camUp, camForward, baseScale * 0.32f, angle
                );
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    dropMat,
                    glm::vec4(0.75f, 0.94f, 1.00f, 0.85f * castFade),
                    KIND_WATER_DROP
                );
            }
        }

        // ------------------------------------------------------------------
        // PHASE 2: PROJECTILE (Undalux aerated water bolt: core + outer shimmer)
        // ------------------------------------------------------------------
        glm::mat4 coreMat = glm::translate(glm::mat4(1.0f), pos);
        coreMat = glm::rotate(coreMat, t * 4.5f + seed, vel);
        coreMat = glm::scale(coreMat, glm::vec3(baseScale * 0.65f, baseScale * 0.65f, baseScale * 1.15f));
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            coreMat,
            glm::vec4(0.78f, 0.94f, 1.00f, 0.92f),
            KIND_WATER_ORB
        );

        glm::mat4 outerMat = glm::translate(glm::mat4(1.0f), pos);
        outerMat = glm::rotate(outerMat, -t * 6.0f + seed, glm::normalize(vel + glm::vec3(0.15f, 0.25f, 0.1f)));
        outerMat = glm::scale(outerMat, glm::vec3(baseScale * 0.95f));
        renderer.submit(
            m_fireOrbMesh,
            m_whiteMaterial,
            outerMat,
            glm::vec4(0.85f, 0.96f, 1.00f, 0.48f),
            KIND_WATER_ORB
        );

        // ------------------------------------------------------------------
        // PHASE 2.3: TRAIL (Gotas de agua + bruma desde historial de combatFrames)
        // ------------------------------------------------------------------
        constexpr int WATER_TRAIL_TICKS = 16;
        const int trailStart = (std::max)(0, currentFrame - WATER_TRAIL_TICKS);
        for (int f = currentFrame - 1; f >= trailStart; --f) {
            const int back = currentFrame - f;
            const float ratio = 1.0f - static_cast<float>(back) / static_cast<float>(WATER_TRAIL_TICKS + 1);
            for (size_t pIdx = 0; pIdx < combatFrames[f].size(); ++pIdx) {
                const Entidad& prevE = combatFrames[f][pIdx];
                if (!prevE.isWaterBolt) continue;
                if (e.poolIndex >= 0 && prevE.poolIndex != e.poolIndex) continue;

                const float h = gx_hash11(static_cast<float>(back) * 3.17f + seed);
                const glm::vec3 jitter(
                    (h - 0.5f) * 0.22f,
                    -0.02f * static_cast<float>(back) * 0.35f,
                    (gx_hash11(static_cast<float>(back) * 7.13f + seed) - 0.5f) * 0.22f
                );

                // Gotas de spray
                const glm::mat4 dropMat = makeRotatedBillboardMatrix(
                    prevE.transform.pos + jitter,
                    camRight,
                    camUp,
                    camForward,
                    baseScale * (0.28f * ratio + 0.08f),
                    h * GX_TAU + t * 3.0f
                );
                renderer.submit(
                    m_fireFlashMesh,
                    m_whiteMaterial,
                    dropMat,
                    glm::vec4(0.80f, 0.95f, 1.00f, 0.78f * ratio),
                    KIND_WATER_DROP
                );

                // Bruma de agua en frames alternos
                if (back % 2 == 1) {
                    const glm::mat4 mistMat = makeRotatedBillboardMatrix(
                        prevE.transform.pos,
                        camRight,
                        camUp,
                        camForward,
                        baseScale * (0.75f * ratio + 0.15f),
                        seed + static_cast<float>(back) * 0.6f
                    );
                    renderer.submit(
                        m_fireSmokeMesh,
                        m_whiteMaterial,
                        mistMat,
                        glm::vec4(0.70f, 0.92f, 0.98f, 0.34f * ratio),
                        KIND_WATER_MIST
                    );
                }
                break;
            }
        }

        // Luz azulada del proyectil
        renderer.submitLight({
            pos,
            baseScale * 3.6f,
            WATER_LIGHT_COLOR,
            1.75f
        });
    }
}

void IceLanceVFX::submitWaterImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward) const
{
    if (!m_fireOrbMesh || !m_fireFlashMesh || !m_fireDecalMesh || !m_fireSmokeMesh || !m_whiteTexture) return;
    (void)camPos;

    const glm::vec3 WATER_LIGHT_COLOR(0.32f, 0.78f, 0.96f);
    const float duration = isBounceOnly ? 0.40f : 1.15f;
    const float progress = gx_clamp(age / duration, 0.0f, 1.0f);
    if (progress >= 1.0f) return;

    const glm::vec3 normal = (glm::length(impact.normal) > 0.001f)
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const float scaleMult = isBounceOnly ? 0.58f : 1.0f;
    const float alphaMult = isBounceOnly ? 0.65f : 1.0f;
    const float seed = gx_hash11(impact.position.x * 11.3f + impact.position.z * 19.7f) * 100.0f;

    // 1. Onda expansiva superficial de agua
    const float ringExpand = 0.40f + gx_smoothstep(0.0f, 0.60f, progress) * 2.25f;
    const float ringFade = 1.0f - gx_smoothstep(0.15f, 1.0f, progress);
    const glm::mat4 decalMat = glm::rotate(
        makeAxisAlignedMatrix(impact.position + normal * 0.04f, normal, 1.25f * scaleMult * ringExpand),
        age * 4.5f + seed,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    renderer.submit(
        m_fireDecalMesh,
        m_whiteMaterial,
        decalMat,
        glm::vec4(0.68f, 0.92f, 0.98f, 0.75f * ringFade * alphaMult),
        KIND_WATER_MIST
    );

    const glm::vec3 upHint = (std::abs(normal.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(upHint, normal));
    const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

    // 2. Abanico de gotas salpicando en parábola (undalux_splash.gdshader)
    const int dropCount = isBounceOnly ? 7 : 14;
    for (int i = 0; i < dropCount; ++i) {
        const float h1 = gx_hash11(seed + static_cast<float>(i) * 2.31f);
        const float h2 = gx_hash11(seed + static_cast<float>(i) * 5.73f);
        const float angle = h1 * GX_TAU;
        const float speed = (2.8f + h2 * 4.2f) * scaleMult;
        const glm::vec3 dir = glm::normalize(normal * (0.55f + h2 * 0.45f) + (std::cos(angle) * tangent + std::sin(angle) * bitangent) * 0.85f);
        glm::vec3 dropPos = impact.position + normal * 0.10f + dir * (speed * age);
        dropPos.y -= 0.5f * 7.0f * age * age;
        if (dropPos.y < impact.position.y - 0.05f && normal.y > 0.5f) continue;

        const float dropFade = 1.0f - gx_smoothstep(0.25f, 0.95f, progress);
        if (dropFade <= 0.01f) continue;

        const glm::mat4 dropMat = makeRotatedBillboardMatrix(
            dropPos,
            camRight,
            camUp,
            camForward,
            (0.20f + h1 * 0.24f) * scaleMult,
            angle + age * 4.0f
        );
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            dropMat,
            glm::vec4(0.82f, 0.96f, 1.00f, 0.90f * dropFade * alphaMult),
            KIND_WATER_DROP
        );
    }

    // 3. Velo de bruma que persiste tras caer las gotas
    const int mistCount = isBounceOnly ? 2 : 5;
    for (int i = 0; i < mistCount; ++i) {
        const float angle = seed + static_cast<float>(i) * (GX_TAU / static_cast<float>(mistCount));
        const glm::vec3 dir = glm::normalize(normal * 0.40f + (std::cos(angle) * tangent + std::sin(angle) * bitangent) * 0.90f);
        const float dist = dampedDistance(age, isBounceOnly ? 1.8f : 3.2f, 2.5f) * scaleMult;
        const glm::vec3 mistPos = impact.position + normal * 0.16f + dir * dist;
        const float mistFade = 1.0f - gx_smoothstep(0.25f, 1.0f, progress);
        if (mistFade <= 0.01f) continue;

        const glm::mat4 mistMat = makeRotatedBillboardMatrix(
            mistPos,
            camRight,
            camUp,
            camForward,
            (0.60f + progress * 0.85f) * scaleMult,
            seed + static_cast<float>(i)
        );
        renderer.submit(
            m_fireSmokeMesh,
            m_whiteMaterial,
            mistMat,
            glm::vec4(0.76f, 0.94f, 0.99f, 0.42f * mistFade * alphaMult),
            KIND_WATER_MIST
        );
    }

    // 4. Luz breve de impacto
    const float flashDur = isBounceOnly ? 0.14f : 0.30f;
    if (age < flashDur) {
        const float lFade = 1.0f - age / flashDur;
        renderer.submitLight({
            impact.position + normal * 0.40f,
            isBounceOnly ? 3.0f : 5.2f,
            WATER_LIGHT_COLOR,
            (isBounceOnly ? 1.4f : 3.0f) * lFade * lFade
        });
    }
}

void IceLanceVFX::submitDaggerCastAndFlight(
    Renderer& renderer,
    const std::vector<std::vector<Entidad>>& combatFrames,
    int currentFrame,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward,
    double timeSeconds) const
{
    if (currentFrame < 0 || currentFrame >= static_cast<int>(combatFrames.size())) return;
    if (m_daggerMesh == INVALID_MESH || !m_fireFlashMesh || !m_whiteTexture) return;
    (void)camPos;

    const glm::vec3 DAGGER_LIGHT_COLOR(0.92f, 0.95f, 1.00f);
    const auto& frameEntities = combatFrames[currentFrame];

    // Offsets de formación compacta para 7 cuchillos (1 central + 3 a cada lado)
    constexpr int KNIFE_COUNT = 7;
    static constexpr float kSideOffsets[KNIFE_COUNT] = { -0.52f, -0.35f, -0.18f, 0.0f, 0.18f, 0.35f, 0.52f };
    static constexpr float kUpOffsets[KNIFE_COUNT]   = { -0.10f,  0.14f, -0.06f, 0.10f, -0.06f, 0.14f, -0.10f };
    static constexpr float kBackOffsets[KNIFE_COUNT] = { -0.28f, -0.18f, -0.08f, 0.0f, -0.08f, -0.18f, -0.28f };

    for (size_t eIdx = 0; eIdx < frameEntities.size(); ++eIdx) {
        const Entidad& e = frameEntities[eIdx];
        if (!e.isDaggerBarrage) continue;

        const glm::vec3 pos = e.transform.pos;
        const glm::vec3 fwd = directionFromPitchYaw(e.transform.rot);
        const glm::vec3 upHint = (std::abs(fwd.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
        const glm::vec3 right = glm::normalize(glm::cross(upHint, fwd));
        const glm::vec3 up = glm::normalize(glm::cross(fwd, right));

        const float t = static_cast<float>(timeSeconds);
        const float seed = static_cast<float>(e.poolIndex >= 0 ? e.poolIndex * 73 : static_cast<int>(eIdx) * 43) * 0.37f;
        const float knifeScale = 3.2f;

        // ------------------------------------------------------------------
        // PHASE 1: CAST (destello blanco + líneas de salida)
        // ------------------------------------------------------------------
        if (e.animationAge <= 0.008f) {
            const float castProgress = gx_clamp(e.animationAge / 0.008f, 0.0f, 1.0f);
            const float castFade = 1.0f - castProgress;
            const glm::mat4 flashMat = makeRotatedBillboardMatrix(
                pos, camRight, camUp, camForward, 1.4f * (0.7f + castProgress * 0.6f), t * 6.0f + seed
            );
            renderer.submit(
                m_fireFlashMesh,
                m_whiteMaterial,
                flashMat,
                glm::vec4(0.95f, 0.97f, 1.00f, 0.90f * castFade),
                KIND_FIRE_FLASH
            );
        }

        // ------------------------------------------------------------------
        // PHASE 2: 7 CUCHILLOS EN FORMACIÓN + ROTACIÓN INDEPENDIENTE
        // ------------------------------------------------------------------
        const glm::mat4 orientMat(
            glm::vec4(right, 0.0f),
            glm::vec4(up,    0.0f),
            glm::vec4(fwd,   0.0f),
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)
        );

        for (int k = 0; k < KNIFE_COUNT; ++k) {
            const glm::vec3 kPos = pos + right * kSideOffsets[k] + up * kUpOffsets[k] + fwd * kBackOffsets[k];
            const float roll = t * (12.0f + static_cast<float>(k) * 0.9f) + seed + static_cast<float>(k) * 1.1f;

            glm::mat4 model = glm::translate(glm::mat4(1.0f), kPos) * orientMat;
            model = glm::rotate(model, roll, glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(knifeScale));

            renderer.submit(
                m_daggerMesh,
                m_daggerMaterial,
                model,
                glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                0
            );
        }

        // ------------------------------------------------------------------
        // PHASE 2.3: TRAIL PLATEADO POR CADA CUCHILLO (historial combatFrames)
        // ------------------------------------------------------------------
        constexpr int DAGGER_TRAIL_TICKS = 10;
        const int trailStart = (std::max)(0, currentFrame - DAGGER_TRAIL_TICKS);
        for (int f = currentFrame - 1; f >= trailStart; --f) {
            const int back = currentFrame - f;
            const float ratio = 1.0f - static_cast<float>(back) / static_cast<float>(DAGGER_TRAIL_TICKS + 1);
            for (size_t pIdx = 0; pIdx < combatFrames[f].size(); ++pIdx) {
                const Entidad& prevE = combatFrames[f][pIdx];
                if (!prevE.isDaggerBarrage) continue;
                if (e.poolIndex >= 0 && prevE.poolIndex != e.poolIndex) continue;

                const glm::vec3 pFwd = directionFromPitchYaw(prevE.transform.rot);
                const glm::vec3 pUpHint = (std::abs(pFwd.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
                const glm::vec3 pRight = glm::normalize(glm::cross(pUpHint, pFwd));
                const glm::vec3 pUp = glm::normalize(glm::cross(pFwd, pRight));

                for (int k = 0; k < KNIFE_COUNT; ++k) {
                    const glm::vec3 tPos = prevE.transform.pos + pRight * kSideOffsets[k] + pUp * kUpOffsets[k] + pFwd * kBackOffsets[k];
                    const glm::mat4 streakMat = makeRotatedBillboardMatrix(
                        tPos, camRight, camUp, camForward, 0.22f * ratio + 0.06f, seed + static_cast<float>(k)
                    );
                    renderer.submit(
                        m_fireFlashMesh,
                        m_whiteMaterial,
                        streakMat,
                        glm::vec4(0.85f, 0.90f, 0.98f, 0.55f * ratio),
                        KIND_FIRE_FLASH
                    );
                }
                break;
            }
        }

        renderer.submitLight({
            pos,
            3.2f,
            DAGGER_LIGHT_COLOR,
            1.4f
        });
    }
}

void IceLanceVFX::submitDaggerImpact(
    Renderer& renderer,
    const ImpactVFX& impact,
    float age,
    bool isBounceOnly,
    const glm::vec3& camPos,
    const glm::vec3& camRight,
    const glm::vec3& camUp,
    const glm::vec3& camForward) const
{
    if (!m_fireFlashMesh || !m_fireDecalMesh || !m_fireSmokeMesh || !m_whiteTexture) return;
    (void)camPos;

    const glm::vec3 DAGGER_LIGHT_COLOR(0.95f, 0.97f, 1.00f);
    const float duration = isBounceOnly ? 0.40f : 1.10f;
    const float progress = gx_clamp(age / duration, 0.0f, 1.0f);
    if (progress >= 1.0f) return;

    const glm::vec3 normal = (glm::length(impact.normal) > 0.001f)
        ? glm::normalize(impact.normal)
        : glm::vec3(0.0f, 1.0f, 0.0f);

    const float scaleMult = isBounceOnly ? 0.58f : 1.0f;
    const float alphaMult = isBounceOnly ? 0.65f : 1.0f;
    const float seed = gx_hash11(impact.position.x * 17.3f + impact.position.z * 23.1f) * 100.0f;

    // 1. Destello blanco intenso y breve
    if (progress < 0.22f) {
        const float fFade = 1.0f - (progress / 0.22f);
        const glm::mat4 flashMat = makeRotatedBillboardMatrix(
            impact.position + normal * 0.15f,
            camRight, camUp, camForward,
            1.6f * scaleMult * (0.7f + progress * 1.5f),
            seed + age * 8.0f
        );
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            flashMat,
            glm::vec4(1.00f, 0.98f, 0.92f, 0.95f * fFade * alphaMult),
            KIND_FIRE_FLASH
        );
    }

    // 2. Onda expansiva superficial orientada por normal
    const float ringExpand = 0.40f + gx_smoothstep(0.0f, 0.55f, progress) * 2.10f;
    const float ringFade = 1.0f - gx_smoothstep(0.12f, 0.90f, progress);
    const glm::mat4 decalMat = glm::rotate(
        makeAxisAlignedMatrix(impact.position + normal * 0.04f, normal, 1.15f * scaleMult * ringExpand),
        age * 5.0f + seed,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    renderer.submit(
        m_fireDecalMesh,
        m_whiteMaterial,
        decalMat,
        glm::vec4(0.88f, 0.92f, 0.98f, 0.72f * ringFade * alphaMult),
        KIND_WIND_DECAL
    );

    // 3. Chispas metálicas y líneas cortantes
    const glm::vec3 upHint = (std::abs(normal.y) > 0.9f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 tangent = glm::normalize(glm::cross(upHint, normal));
    const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

    const int sparkCount = isBounceOnly ? 6 : 12;
    for (int i = 0; i < sparkCount; ++i) {
        const float h1 = gx_hash11(seed + static_cast<float>(i) * 3.17f);
        const float angle = h1 * GX_TAU;
        const float speed = (3.5f + h1 * 4.5f) * scaleMult;
        const glm::vec3 dir = glm::normalize(normal * 0.55f + (std::cos(angle) * tangent + std::sin(angle) * bitangent) * 0.85f);
        const glm::vec3 sPos = impact.position + normal * 0.12f + dir * (speed * age);
        const float sFade = 1.0f - gx_smoothstep(0.15f, 0.85f, progress);
        if (sFade <= 0.01f) continue;

        const glm::mat4 sparkMat = makeRotatedBillboardMatrix(
            sPos, camRight, camUp, camForward,
            (0.28f + h1 * 0.25f) * scaleMult,
            angle + age * 6.0f
        );
        renderer.submit(
            m_fireFlashMesh,
            m_whiteMaterial,
            sparkMat,
            glm::vec4(1.00f, 0.90f, 0.68f, 0.90f * sFade * alphaMult),
            KIND_FIRE_FLASH
        );
    }

    // 4. Polvo breve
    const int dustCount = isBounceOnly ? 2 : 4;
    for (int i = 0; i < dustCount; ++i) {
        const float angle = seed + static_cast<float>(i) * (GX_TAU / static_cast<float>(dustCount));
        const glm::vec3 dir = glm::normalize(normal * 0.35f + (std::cos(angle) * tangent + std::sin(angle) * bitangent) * 0.90f);
        const glm::vec3 dPos = impact.position + normal * 0.15f + dir * (dampedDistance(age, 2.5f, 2.8f) * scaleMult);
        const float dFade = 1.0f - gx_smoothstep(0.20f, 1.0f, progress);
        if (dFade <= 0.01f) continue;

        const glm::mat4 dustMat = makeRotatedBillboardMatrix(
            dPos, camRight, camUp, camForward,
            (0.50f + progress * 0.70f) * scaleMult,
            seed + static_cast<float>(i)
        );
        renderer.submit(
            m_fireSmokeMesh,
            m_whiteMaterial,
            dustMat,
            glm::vec4(0.75f, 0.78f, 0.82f, 0.34f * dFade * alphaMult),
            KIND_FIRE_SMOKE
        );
    }

    // 5. Luz breve
    const float flashDur = isBounceOnly ? 0.12f : 0.24f;
    if (age < flashDur) {
        const float lFade = 1.0f - age / flashDur;
        renderer.submitLight({
            impact.position + normal * 0.40f,
            isBounceOnly ? 2.8f : 4.8f,
            DAGGER_LIGHT_COLOR,
            (isBounceOnly ? 1.5f : 3.2f) * lFade * lFade
        });
    }
}
