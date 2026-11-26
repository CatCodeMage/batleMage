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

SceneCamera CameraPath::cameraAtFrame(int frame) const {
	auto startsAfterFrame = [frame](const CameraShot& shot) { return shot.startFrame > frame; };
	auto firstLaterShot = std::find_if(w_shots.begin(), w_shots.end(), startsAfterFrame);
	return std::prev(firstLaterShot)->camera;   // el anterior empieza en frame o antes; existe porque hay plano en el 0
} 