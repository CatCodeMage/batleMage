#pragma once
// Adaptacion a OpenGL 3.3 C++ del VFX procedural de Ice Lance e Impact Shards de GLACILUX Lite.
// Basado en glacilux_meshes.gd, glacilux_projectile_3d.gd y glacilux_impact_3d.gd
// Copyright (c) 2026 gennai (https://gennai.itch.io) — MIT License.

#include "global.h"
#include "MeshLibrary.h"
#include "TextureLibrary.h"
#include "Renderer.h"
#include "Entidad.h"
#include "camera.h"
#include "MaterialLibrary.h"

class IceLanceVFX {
public:
    void init(MeshLibrary& meshes, const TextureLibrary& textures, MaterialLibrary& matLib);

    void submitFrame(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        const std::vector<ImpactVFX>& impacts,
        int currentFrame,
        const Camera& camera,
        double timeSeconds) const;

private:

    MaterialId m_whiteMaterial = INVALID_MATERIAL_ID;
    MaterialId m_daggerMaterial = INVALID_MATERIAL_ID;

    void submitFlightAndTrails(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitImpacts(
        Renderer& renderer,
        const std::vector<ImpactVFX>& impacts,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitSingleImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitFireCastAndFlight(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitFireImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward) const;

    void submitLightningCastAndFlight(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitLightningImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitEarthCastAndFlight(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitEarthImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward) const;

    void submitWindCastAndFlight(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitWindImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward) const;

    void submitWaterCastAndFlight(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitWaterImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward) const;

    void submitDaggerCastAndFlight(
        Renderer& renderer,
        const std::vector<std::vector<Entidad>>& combatFrames,
        int currentFrame,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward,
        double timeSeconds) const;

    void submitDaggerImpact(
        Renderer& renderer,
        const ImpactVFX& impact,
        float age,
        bool isBounceOnly,
        const glm::vec3& camPos,
        const glm::vec3& camRight,
        const glm::vec3& camUp,
        const glm::vec3& camForward) const;

    MeshId m_lanceBodyMesh   = INVALID_MESH;
    MeshId m_daggerMesh      = INVALID_MESH;
    GLuint m_daggerTexture   = 0;
    MeshId m_crownSpikeMesh  = INVALID_MESH;
    MeshId m_shardBoxMesh    = INVALID_MESH;
    MeshId m_shardWedgeMesh  = INVALID_MESH;
    MeshId m_frostQuadMesh   = INVALID_MESH;
    MeshId m_mistQuadMesh    = INVALID_MESH;
    MeshId m_sparkleQuadMesh = INVALID_MESH;
    MeshId m_fireDecalMesh   = INVALID_MESH;
    MeshId m_fireSmokeMesh   = INVALID_MESH;
    MeshId m_fireOrbMesh     = INVALID_MESH;
    MeshId m_fireFlameMesh   = INVALID_MESH;
    MeshId m_fireFlashMesh   = INVALID_MESH;
    MeshId m_earthRockMesh   = INVALID_MESH;
    GLuint m_whiteTexture    = 0;
};
