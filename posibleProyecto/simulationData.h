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
#include "simulationData.h"

struct FramesData
{
    std::vector<std::vector <Entidad>> combat;

    int frame;
    int lessFrames;
};

struct simulationData
{
    void init(GLFWwindow* window);

    void changeLights(std::vector<SceneLight>& newLights);

    void clearLastFrame(); 
    
    void updateFrame(GLFWwindow* window);

    void procesMainBodyLoopRealTime(GLFWwindow* window, SystemInfo& sysInfo);

    void destroy();
    
    // Data
    MeshLibrary libMesh;
    ModelLibrary libModel;
    TextureLibrary textures;
    MapData map;
    std::vector<SceneLight> lights;
    Camera viewCamera;
    GLuint program;
    Renderer renderer;
    ShadowSystem shadows;
    Input keysInput;
    FramesData framesInfo;
};

