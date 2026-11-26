#pragma once
#include <vector>
#include "SceneCamera.h"

enum class CameraPathMode { Recording, Playback };   // Edicion / Resultado en la interfaz

struct CameraShot {
	int startFrame;
	SceneCamera camera;   // copia: si luego se retoca la vista, el plano guardado no cambia
};

class CameraPath {
public:
	void saveShot(int frame, const SceneCamera& camera);   // si el frame ya tiene plano, lo sustituye
	void removeShot(int shotIndex);
	SceneCamera cameraAtFrame(int frame) const;
	const std::vector<CameraShot>& getShots() const { return w_shots; }
private:
	std::vector<CameraShot> w_shots;   // ordenados por startFrame; el primero siempre empieza en 0
};