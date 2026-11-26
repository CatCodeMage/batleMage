#include "CombatPanel.h"
#include "simulationData.h"
#include <imgui.h>

void drawCombatPanel(FramesData& framesInfo) {
	const int lastFrame = static_cast<int>(framesInfo.combat.size()) - 1;
	ImGui::Text("%.0f FPS", ImGui::GetIO().Framerate);

	ImGui::Begin("Combate");
	ImGui::Checkbox("Pausa", &framesInfo.isPaused);
	ImGui::SliderInt("Frame", &framesInfo.frame, 0, lastFrame);

	ImGui::InputInt("Frame exacto", &framesInfo.frame);
	const bool isTypingFrame = ImGui::IsItemActive();   // tambien cuenta mantener pulsado + o -
	if (isTypingFrame) framesInfo.isPaused = true;       // si no, el combate avanza mientras se escribe
	framesInfo.frame = std::clamp(framesInfo.frame, 0, lastFrame);   // InputInt no tiene limites
	ImGui::End();
}