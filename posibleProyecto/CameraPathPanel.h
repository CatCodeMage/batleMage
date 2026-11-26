#pragma once
#include "CameraPath.h"
#include "SceneMosaic.h"
struct FramesData;

void drawCameraPathPanel(CameraPathMode& mode, CameraPath& path, SceneCamera& selectedCamera, FramesData& framesInfo,
	SceneMosaic& mosaic, const std::vector<int>& characterIds);