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
#include "UserInterface.h"
#include "CameraPath.h"
#include "SceneMosaic.h"
#include "SceneCamera.h"
#include <array>
#include "SceneCameraPanel.h"
#include "CameraPathPanel.h"

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
    UserInterface visualInterface;
    InteractionMode interactionMode = InteractionMode::Editing;
    CameraPath cameraPath;
    CameraPathMode cameraPathMode = CameraPathMode::Recording;
    SceneMosaic sceneMosaic;
    std::array<SceneCamera, SceneMosaic::MAX_VIEWS> sceneCameras;
    std::vector<int> characterIds;   // los combatientes del replay, para elegir a quien sigue una camara
    int selectedViewIndex = 0;
    int freeCameraView = 0;          // la vista que mueve viewCamera (la ultima libre controlada)
private:
    struct ReplayAnimator {
        ModelId model;
        bm::anim::Animator animator;
        const bm::anim::AnimationClip* clip = nullptr;
        bool looping = true;
        double seconds = 0.0;
    };

    void prepareLightingPrograms();
    void drawSceneViews(const std::vector<SceneViewport>& views);
    void drawSceneInViewport(const SceneViewport& viewport, const CameraView& cameraView);
    CameraView cameraViewForView(int viewIndex, const std::vector<Entidad>& frameEntities) const;

    const bm::anim::Animator& poseFor(const Entidad& entity, const SkinnedModel& model);
    void submitEntities(const std::vector<Entidad>& entities);

    void createDefaultSceneCameras();

    std::vector<SceneViewport> currentViews(const SystemInfo& sysInfo) const;
    void updateViewSelection(GLFWwindow* window, const std::vector<SceneViewport>& views, int framebufferHeight);
    void startControllingCamera(GLFWwindow* window, int viewIndex);
    void enterFullScreen(GLFWwindow* window);

    void handleCameraPathPanelAction(CameraPathPanelAction action);
    void saveMontageFile() const;
    void loadMontageFile();

    bm::anim::SkinnedRenderer skinnedRenderer;
    std::unordered_map<int, ReplayAnimator> animators;
};

