#pragma once
#include <array>
#include <vector>
#include "SceneCamera.h"
#include "SceneMosaic.h"

using SceneCameras = std::array<SceneCamera, SceneMosaic::MAX_VIEWS>;

void drawViewHeaders(SceneCameras& cameras, const std::vector<SceneViewport>& views, int selectedView, int framebufferHeight);
void drawSelectedCameraPanel(SceneCamera& camera, int viewIndex, const std::vector<int>& characterIds);

std::string characterName(int characterId);
void drawCharacterSelector(const char* label, int& targetCharacterId, const std::vector<int>& characterIds);