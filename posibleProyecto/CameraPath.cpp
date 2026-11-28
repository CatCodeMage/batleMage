#include "CameraPath.h"
#include <algorithm>
#include <iterator>

void CameraPath::saveShot(int frame, const CameraPose& pose) { // Guarda manteniendo el orden
	auto startsAtOrAfterFrame = [frame](const CameraShot& shot) { return shot.startFrame >= frame; };
	auto insertPosition = std::find_if(w_shots.begin(), w_shots.end(), startsAtOrAfterFrame);
	const bool hasShotAtFrame = insertPosition != w_shots.end() && insertPosition->startFrame == frame;

	if (hasShotAtFrame) insertPosition->pose = pose;
	else                w_shots.insert(insertPosition, { frame, pose });   
}

void CameraPath::removeShot(int shotIndex) {
	const bool isDefaultShot = shotIndex == 0;   // la del frame 0 solo se sustituye: siempre tiene que haber camara
	const bool isOutOfRange = shotIndex < 0 || shotIndex >= (int)w_shots.size();
	if (isDefaultShot || isOutOfRange) return;
	w_shots.erase(w_shots.begin() + shotIndex);
}

CameraPose CameraPath::poseAtFrame(int frame) const {
	auto startsAfterFrame = [frame](const CameraShot& shot) { return shot.startFrame > frame; };
	auto firstLaterShot = std::find_if(w_shots.begin(), w_shots.end(), startsAfterFrame);
	return std::prev(firstLaterShot)->pose;   // la anterior empieza en frame o antes; existe porque hay toma en el 0
}

