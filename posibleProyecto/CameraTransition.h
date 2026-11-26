#pragma once
#include <vector>
#include "SceneCamera.h"

constexpr int DEFAULT_TRANSITION_FRAMES = 60;   // 1 s: el replay va a 60 frames por segundo

enum class TransitionType { Cut, CameraMove };   // los fundidos llegan en los pasos 5b y 5c
enum class CameraMoveFocus { Natural, Character, CombatCenter };

const char* const TRANSITION_TYPE_NAMES[] = { "Corte", "Movimiento de camara" };            // mismo orden que TransitionType
const char* const CAMERA_MOVE_FOCUS_NAMES[] = { "Natural", "Un mago", "Centro del combate" };   // mismo orden que CameraMoveFocus

struct ShotTransition {   // como se pasa de un plano al siguiente: ocupa los ultimos frames del plano
	TransitionType  type = TransitionType::Cut;
	int             durationFrames = DEFAULT_TRANSITION_FRAMES;
	CameraMoveFocus focus = CameraMoveFocus::Natural;
	int             focusCharacterId = NO_CHARACTER;
	float           arcHeight = 0.f;   // cuanto sube a mitad de camino, p. ej. para no atravesar columnas
};

// progress: 0 = todavia el plano de origen; 1 = ya el de destino
CameraView blendCameraMove(const CameraView& from, const CameraView& to, float progress,
	const ShotTransition& transition, const std::vector<Entidad>& frameEntities);