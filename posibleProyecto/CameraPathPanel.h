#pragma once
#include "CameraPath.h"
#include "SceneMosaic.h"
struct FramesData;

enum class CameraPathPanelAction { None, SaveMontage, LoadMontage };   // lo hace quien llama: el panel no sabe de ficheros

CameraPathPanelAction drawCameraPathPanel(CameraPathMode& mode, CameraPath& path, SceneCamera& selectedCamera, FramesData& framesInfo,
	SceneMosaic& mosaic, const std::vector<int>& characterIds);