#pragma once
#include "CameraPath.h"
#include "SceneMosaic.h"
struct FramesData;

void drawCameraPathPanel(CameraPathMode& mode, CameraPath& path, Camera& camera, FramesData& framesInfo, SceneMosaic& mosaic);