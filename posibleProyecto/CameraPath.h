#pragma once
#include <vector>
#include "CameraTransition.h"

enum class CameraPathMode { Recording, Playback };   // Edicion / Resultado en la interfaz

struct CameraShot {
	int startFrame;
	SceneCamera camera;           // copia: si luego se retoca la vista, el plano guardado no cambia
	ShotTransition transition;    // hacia el plano siguiente
};

class CameraPath {
public:
	void saveShot(int frame, const SceneCamera& camera);   // si el frame ya tiene plano, lo sustituye
	void removeShot(int shotIndex);
	const std::vector<CameraShot>& getShots() const { return w_shots; }

	ShotTransition& transitionOf(int shotIndex) { return w_shots[shotIndex].transition; }
	CameraView viewAtFrame(int frame, const std::vector<Entidad>& frameEntities) const;   // con la transicion si toca

private:
	int shotIndexAtFrame(int frame) const;
	std::vector<CameraShot> w_shots;   // ordenados por startFrame; el primero siempre empieza en 0
};