#include "CameraPathPanel.h"
#include "SceneCameraPanel.h"
#include "simulationData.h"
#include <imgui.h>

namespace {
	constexpr int MIN_TRANSITION_FRAMES = 1;
	constexpr int MAX_TRANSITION_FRAMES = 240;   // 4 s a 60 frames por segundo
	constexpr float MAX_ARC_HEIGHT = 20.f;

	void drawTransitionSettings(ShotTransition& transition, const std::vector<int>& characterIds) {
		int typeIndex = (int)transition.type;
		if (ImGui::Combo("Transicion", &typeIndex, TRANSITION_TYPE_NAMES, IM_ARRAYSIZE(TRANSITION_TYPE_NAMES)))
			transition.type = (TransitionType)typeIndex;
		if (transition.type == TransitionType::Cut) return;

		ImGui::SliderInt("Duracion (frames)", &transition.durationFrames, MIN_TRANSITION_FRAMES, MAX_TRANSITION_FRAMES);

		int focusIndex = (int)transition.focus;
		if (ImGui::Combo("Mirar a", &focusIndex, CAMERA_MOVE_FOCUS_NAMES, IM_ARRAYSIZE(CAMERA_MOVE_FOCUS_NAMES))) {
			transition.focus = (CameraMoveFocus)focusIndex;
			const bool needsDefaultCharacter = transition.focus == CameraMoveFocus::Character
				&& transition.focusCharacterId == NO_CHARACTER && !characterIds.empty();
			if (needsDefaultCharacter) transition.focusCharacterId = characterIds.front();
		}
		if (transition.focus == CameraMoveFocus::Character)
			drawCharacterSelector("Mago", transition.focusCharacterId, characterIds);

		ImGui::SliderFloat("Arco", &transition.arcHeight, 0.f, MAX_ARC_HEIGHT, "%.1f");
	}

	void drawModeSelector(CameraPathMode& mode) {
		if (ImGui::RadioButton("Edicion", mode == CameraPathMode::Recording)) mode = CameraPathMode::Recording;
		ImGui::SameLine();
		if (ImGui::RadioButton("Resultado", mode == CameraPathMode::Playback)) mode = CameraPathMode::Playback;
	}

	void drawShotList(CameraPath& path, SceneCamera& selectedCamera, FramesData& framesInfo, const std::vector<int>& characterIds) {
		constexpr int NO_SHOT = -1;
		int shotToRemove = NO_SHOT;
		const std::vector<CameraShot>& shots = path.getShots();

		for (int shotIndex = 0; shotIndex < (int)shots.size(); ++shotIndex) {
			const CameraShot& shot = shots[shotIndex];
			ImGui::PushID(shotIndex);   // todas las filas tienen "Ir" y "Borrar": sin ID distinto, ImGui las confunde
			ImGui::Text("Frame %d  %s", shot.startFrame, sceneCameraTypeName(shot.camera.type));
			ImGui::SameLine();
			if (ImGui::SmallButton("Ir")) {
				framesInfo.frame = shot.startFrame;
				selectedCamera = shot.camera;   // la vista seleccionada pasa a tener este plano, para retocarlo
			}
			const bool isDefaultShot = shotIndex == 0;
			if (!isDefaultShot) {
				ImGui::SameLine();
				if (ImGui::SmallButton("Borrar")) shotToRemove = shotIndex;
			}
			const bool isLastShot = shotIndex + 1 == (int)shots.size();
			if (!isLastShot && ImGui::TreeNode("Transicion al siguiente")) {   // el ultimo no tiene siguiente
				drawTransitionSettings(path.transitionOf(shotIndex), characterIds);
				ImGui::TreePop();
			}
			ImGui::PopID();
		}

		if (shotToRemove != NO_SHOT) path.removeShot(shotToRemove);   // fuera del bucle: borrar dentro invalidaria la lista que se recorre
	}

	void drawMosaicControls(SceneMosaic& mosaic) {
		ImGui::BeginDisabled(!mosaic.canRemoveViews());
		if (ImGui::Button("-")) mosaic.removeViews();
		ImGui::EndDisabled();

		ImGui::SameLine();
		ImGui::Text("%d vistas", mosaic.getViewCount());

		ImGui::SameLine();
		ImGui::BeginDisabled(!mosaic.canAddViews());
		if (ImGui::Button("+")) mosaic.addViews();
		ImGui::EndDisabled();
	}
	
	CameraPathPanelAction drawMontageFileButtons() {
		CameraPathPanelAction requestedAction = CameraPathPanelAction::None;
		if (ImGui::Button("Guardar montaje")) requestedAction = CameraPathPanelAction::SaveMontage;
		ImGui::SameLine();
		if (ImGui::Button("Cargar montaje")) requestedAction = CameraPathPanelAction::LoadMontage;
		return requestedAction;
	}
}



CameraPathPanelAction drawCameraPathPanel(CameraPathMode& mode, CameraPath& path, SceneCamera& selectedCamera, FramesData& framesInfo,
	SceneMosaic& mosaic, const std::vector<int>& characterIds) {
	ImGui::Begin("Camara");
	drawMosaicControls(mosaic);
	ImGui::Separator();
	drawModeSelector(mode);
	ImGui::Separator();
	const CameraPathPanelAction requestedAction = drawMontageFileButtons();
	ImGui::Separator();
	if (mode == CameraPathMode::Recording) ImGui::TextUnformatted("Espacio: guardar la vista seleccionada en este frame");
	drawShotList(path, selectedCamera, framesInfo, characterIds);
	ImGui::End();
	return requestedAction;
}