#pragma once
#include <vector>
#include "camera.h"

enum class CameraPathMode { Recording, Playback };   // Edicion / Resultado en la interfaz

struct CameraShot {
	int startFrame;
	CameraPose pose;
};

class CameraPath {
public:
	void saveShot(int frame, const CameraPose& pose);   // si el frame ya tiene toma, la sustituye
	void removeShot(int shotIndex);
	CameraPose poseAtFrame(int frame) const;
	const std::vector<CameraShot>& getShots() const { return w_shots; }
private:
	std::vector<CameraShot> w_shots;   // ordenadas por startFrame; la primera siempre empieza en 0
};