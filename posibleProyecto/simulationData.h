#pragma once
#include "global.h"
#include "shaders.h"
#include "systemInfo.h"
#include "camera.h"
#include "MeshLibrary.h"
#include "Input.h"
#include "Renderer.h"
#include "Transform.h"
#include "Entidad.h"
#include "drawObjects.h"
#include "Paths.h"
#include "ShadowSystem.h"
#include "AssetLoadData.h"
#include "MapData.h"
#include "webPage.h"
#include "jsonManager.h"

struct simulationData
{
    ~simulationData();
    void init(GLFWwindow* window);

    void changeLights(std::vector<SceneLight>& newLights);

    void clearLastFrame(); 
    
    void updateFrame(GLFWwindow* window);

    void procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo);

    // Data
    MeshLibrary libMesh;
    ModelLibrary libModel;
    TextureLibrary textures;
    MapData map;
    std::vector<SceneLight> lights;
    Camera viewCamera;
    GLuint program = 0;
    Renderer renderer;
    ShadowSystem shadows;
    Input keysInput;
    FramesData framesInfo;

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
    std::unordered_map<int, ReplayAnimator> animators;
};

