#include "CameraPath.h"
#include <algorithm>
#include <iterator> 

void CameraPath::removeShot(int shotIndex) {
	const bool isDefaultShot = shotIndex == 0;   // la del frame 0 solo se sustituye: siempre tiene que haber camara
	const bool isOutOfRange = shotIndex < 0 || shotIndex >= (int)w_shots.size();
	if (isDefaultShot || isOutOfRange) return;
	w_shots.erase(w_shots.begin() + shotIndex);
}

void CameraPath::saveShot(int frame, const SceneCamera& camera) {
	auto startsAtOrAfterFrame = [frame](const CameraShot& shot) { return shot.startFrame >= frame; };
	auto insertPosition = std::find_if(w_shots.begin(), w_shots.end(), startsAtOrAfterFrame);
	const bool hasShotAtFrame = insertPosition != w_shots.end() && insertPosition->startFrame == frame;

	if (hasShotAtFrame) insertPosition->camera = camera;
	else                w_shots.insert(insertPosition, { frame, camera });   // insertar aqui mantiene el orden
}

int CameraPath::shotIndexAtFrame(int frame) const {
	auto startsAfterFrame = [frame](const CameraShot& shot) { return shot.startFrame > frame; };
	auto firstLaterShot = std::find_if(w_shots.begin(), w_shots.end(), startsAfterFrame);
	return (int)std::distance(w_shots.begin(), firstLaterShot) - 1;   // el anterior empieza en frame o antes; existe porque hay plano en el 0
}

CameraView CameraPath::viewAtFrame(int frame, const std::vector<Entidad>& frameEntities) const {
	const int shotIndex = shotIndexAtFrame(frame);
	const CameraShot& shot = w_shots[shotIndex];
	const CameraView shotView = cameraViewFor(shot.camera, frameEntities);

	const bool isLastShot = shotIndex + 1 == (int)w_shots.size();
	if (isLastShot || shot.transition.type == TransitionType::Cut) return shotView;

	const CameraShot& nextShot = w_shots[shotIndex + 1];
	const int shotLength = nextShot.startFrame - shot.startFrame;
	const int transitionFrames = (std::min)(shot.transition.durationFrames, shotLength);   // no puede empezar antes que su propio plano
	const int framesUntilNextShot = nextShot.startFrame - frame;
	if (framesUntilNextShot > transitionFrames) return shotView;

	const float progress = 1.f - (float)framesUntilNextShot / (float)transitionFrames;
	return blendCameraMove(shotView, cameraViewFor(nextShot.camera, frameEntities), progress, shot.transition, frameEntities);
}
