#include "SceneCameraPanel.h"
#include <imgui.h>
#include <string>

namespace {
	constexpr float HEADER_MARGIN_PIXELS = 6.0f;
	constexpr float HEADER_BACKGROUND_ALPHA = 0.6f;
	constexpr float TYPE_COMBO_WIDTH_PIXELS = 170.0f;
	constexpr float SELECTION_BORDER_PIXELS = 3.0f;
	const ImU32 SELECTION_COLOR = IM_COL32(66, 117, 209, 255);   // el azul de acento del tema

	struct SliderRange { float min; float max; };
	constexpr SliderRange FOLLOW_DISTANCE{ 1.f, 20.f };
	constexpr SliderRange FOLLOW_HEIGHT{ 0.f, 15.f };
	constexpr SliderRange SIDE_OFFSET{ -5.f, 5.f };
	constexpr SliderRange EYE_HEIGHT{ 0.5f, 3.f };
	constexpr SliderRange DUEL_MARGIN{ 0.f, 20.f };
	constexpr SliderRange MAP_DISTANCE{ 10.f, 150.f };
	constexpr SliderRange MAP_HEIGHT{ 5.f, 120.f };
	constexpr SliderRange FIELD_OF_VIEW{ 20.f, 100.f };

	struct InterfaceRect { ImVec2 min; ImVec2 max; };

	InterfaceRect toInterfaceRect(const SceneViewport& view, int framebufferHeight) {
		const ImVec2 scale = ImGui::GetIO().DisplayFramebufferScale;
		const float top = (framebufferHeight - view.y - view.height) / scale.y;   // OpenGL cuenta desde abajo; ImGui, desde arriba
		return { ImVec2(view.x / scale.x, top), ImVec2((view.x + view.width) / scale.x, top + view.height / scale.y) };
	}

	void drawSlider(const char* label, float& value, SliderRange range) {
		ImGui::SliderFloat(label, &value, range.min, range.max, "%.1f");
	}

	std::string characterName(int characterId) { return "Mago " + std::to_string(characterId); }

	void drawCharacterSelector(int& targetCharacterId, const std::vector<int>& characterIds) {
		if (!ImGui::BeginCombo("Personaje", characterName(targetCharacterId).c_str())) return;
		for (int characterId : characterIds)
			if (ImGui::Selectable(characterName(characterId).c_str(), characterId == targetCharacterId))
				targetCharacterId = characterId;
		ImGui::EndCombo();
	}

	void drawTypeSelector(SceneCamera& camera) {
		int typeIndex = (int)camera.type;
		ImGui::SetNextItemWidth(TYPE_COMBO_WIDTH_PIXELS);
		const bool hasChangedType = ImGui::Combo("##type", &typeIndex, SCENE_CAMERA_TYPE_NAMES, IM_ARRAYSIZE(SCENE_CAMERA_TYPE_NAMES));
		if (hasChangedType) camera = makeSceneCamera((SceneCameraType)typeIndex, camera.targetCharacterId);   // valores de partida del tipo nuevo
	}

	void drawTypeSettings(SceneCamera& camera) {
		switch (camera.type) {
		case SceneCameraType::FollowBehind:
		case SceneCameraType::FollowFront:
			drawSlider("Distancia", camera.distance, FOLLOW_DISTANCE);
			drawSlider("Altura", camera.height, FOLLOW_HEIGHT);
			drawSlider("Desvio lateral", camera.sideOffset, SIDE_OFFSET);
			break;
		case SceneCameraType::FirstPerson:
			drawSlider("Altura de los ojos", camera.height, EYE_HEIGHT);
			break;
		case SceneCameraType::Duel:
			drawSlider("Margen", camera.distance, DUEL_MARGIN);
			drawSlider("Altura", camera.height, FOLLOW_HEIGHT);
			break;
		case SceneCameraType::WholeMap:
			drawSlider("Distancia", camera.distance, MAP_DISTANCE);
			drawSlider("Altura", camera.height, MAP_HEIGHT);
			break;
		case SceneCameraType::Overhead:
			drawSlider("Altura", camera.height, MAP_HEIGHT);
			break;
		case SceneCameraType::Free:
			ImGui::TextUnformatted("Doble clic en la vista para moverla");
			break;
		}
	}
}

void drawViewHeaders(SceneCameras& cameras, const std::vector<SceneViewport>& views, int selectedView, int framebufferHeight) {
	const ImGuiWindowFlags headerFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
		| ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking
		| ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing;

	for (int viewIndex = 0; viewIndex < (int)views.size(); ++viewIndex) {
		const InterfaceRect rect = toInterfaceRect(views[viewIndex], framebufferHeight);
		if (viewIndex == selectedView)
			ImGui::GetBackgroundDrawList()->AddRect(rect.min, rect.max, SELECTION_COLOR, 0.0f, 0, SELECTION_BORDER_PIXELS);

		const std::string headerName = "##ViewHeader" + std::to_string(viewIndex);   // una ventana por vista: el nombre las distingue
		ImGui::SetNextWindowPos(ImVec2(rect.min.x + HEADER_MARGIN_PIXELS, rect.min.y + HEADER_MARGIN_PIXELS));
		ImGui::SetNextWindowBgAlpha(HEADER_BACKGROUND_ALPHA);
		ImGui::Begin(headerName.c_str(), nullptr, headerFlags);
		drawTypeSelector(cameras[viewIndex]);
		ImGui::End();
	}
}

void drawSelectedCameraPanel(SceneCamera& camera, int viewIndex, const std::vector<int>& characterIds) {
	ImGui::Begin("Vista");
	ImGui::Text("Vista %d: %s", viewIndex + 1, sceneCameraTypeName(camera.type));
	ImGui::Separator();
	if (needsCharacter(camera.type)) drawCharacterSelector(camera.targetCharacterId, characterIds);
	drawTypeSettings(camera);
	drawSlider("Campo de vision", camera.fieldOfViewDegrees, FIELD_OF_VIEW);
	ImGui::End();
}