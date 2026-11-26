#include "CombatPanel.h"
#include "CameraPathPanel.h"
#include "simulationData.h"
#include <imgui.h>
#include <algorithm>

namespace {
	void drawModeSelector(CameraPathMode& mode) {
		if (ImGui::RadioButton("Edicion", mode == CameraPathMode::Recording)) mode = CameraPathMode::Recording;
		ImGui::SameLine();
		if (ImGui::RadioButton("Resultado", mode == CameraPathMode::Playback)) mode = CameraPathMode::Playback;
	}

	void drawShotList(CameraPath& path, Camera& camera, FramesData& framesInfo) {
		constexpr int NO_SHOT = -1;
		int shotToRemove = NO_SHOT;
		const std::vector<CameraShot>& shots = path.getShots();

		for (int shotIndex = 0; shotIndex < (int)shots.size(); ++shotIndex) {
			const CameraShot& shot = shots[shotIndex];
			ImGui::PushID(shotIndex);   // todas las filas tienen "Ir" y "Borrar": sin ID distinto, ImGui las confunde
			ImGui::Text("Frame %d", shot.startFrame);
			ImGui::SameLine();
			if (ImGui::SmallButton("Ir")) {
				framesInfo.frame = shot.startFrame;
				camera.setPose(shot.pose);   // en Edicion deja la camara lista para retocar esta toma
			}
			const bool isDefaultShot = shotIndex == 0;
			if (!isDefaultShot) {
				ImGui::SameLine();
				if (ImGui::SmallButton("Borrar")) shotToRemove = shotIndex;
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
}

void drawCameraPathPanel(CameraPathMode& mode, CameraPath& path, Camera& camera, FramesData& framesInfo, SceneMosaic& mosaic) {
	ImGui::Begin("Camara");
	drawMosaicControls(mosaic);
	ImGui::Separator();
	drawModeSelector(mode);
	ImGui::Separator();
	if (mode == CameraPathMode::Recording) ImGui::TextUnformatted("Espacio: guardar la camara en este frame");
	drawShotList(path, camera, framesInfo);
	ImGui::End();
}
