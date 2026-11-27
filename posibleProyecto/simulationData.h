#pragma once
#include "global.h"
#include "shaders.h"
#include "systemInfo.h"
#include "camera.h"
#include "MeshLibrary.h"
#include "ModelLibrary.h"
#include "Input.h"
#include "Renderer.h"
#include "Transform.h"
#include "Entidad.h"
#include "drawObjects.h"
#include "Paths.h"
#include "ShadowSystem.h"
#include "AssetLoadData.h"
#include "MapData.h"
#include "jsonManager.h"
#include "UserInterface.h"
#include "CameraPath.h"
#include "Mp4Exporter.h"
#include "MaterialLibrary.h"
#include "sistema_particulas/ParticleModule.h"
#include "IceLanceVFX.h"

enum class InteractionMode { Editing, ControllingCamera };

struct simulationData
{
    ~simulationData();
    void init(GLFWwindow* window);

    void changeLights(std::vector<SceneLight>& newLights);

    void clearLastFrame(GLFWwindow* window, SystemInfo& sysInfo);
    
    void updateFrame(GLFWwindow* window);

    void procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo);

    void updateInteractionMode(GLFWwindow* window);

    void updateCamera(const SystemInfo& sysInfo);

    void saveShotIfRequested();

    void destroy();

    std::vector<SceneLight> setLights();
    
    // Data
    MeshLibrary libMesh;
    ModelLibrary libModel;
    MaterialLibrary libMaterial;
    TextureLibrary textures;
    MapData map;
    std::vector<SceneLight> lights;
    Camera viewCamera;
    GLuint program = 0;
    Renderer renderer;
    ShadowSystem shadows;
    Input keysInput;
    FramesData framesInfo;
    UserInterface visualInterface;
    InteractionMode interactionMode = InteractionMode::Editing;
    CameraPath cameraPath;
    CameraPathMode cameraPathMode = CameraPathMode::Recording;
    glm::mat4 viewProjection;

private:
    struct ReplayAnimator {
        ModelId model;
        bm::anim::Animator animator;
        const bm::anim::AnimationClip* clip = nullptr;
        bool looping = true;
        double seconds = 0.0;
    };

    const bm::anim::Animator& poseFor(const Entidad& entity, const SkinnedModel& model);
    void submitEntities(const std::vector<Entidad>& entities);

    bm::anim::SkinnedRenderer skinnedRenderer;
    bm::particles::ParticleModule particleModule;
    std::unordered_map<int, ReplayAnimator> animators;
    IceLanceVFX iceLanceVfx;
};

