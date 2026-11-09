#include "CombatPanel.h"
#include "simulationData.h"
#include <imgui.h>

void drawCombatPanel(FramesData& framesInfo) {
	const int lastFrame = static_cast<int>(framesInfo.combat.size()) - 1;

	ImGui::Begin("Combate");
	ImGui::Checkbox("Pausa", &framesInfo.isPaused);
	ImGui::SliderInt("Frame", &framesInfo.frame, 0, lastFrame);
	ImGui::End();
}