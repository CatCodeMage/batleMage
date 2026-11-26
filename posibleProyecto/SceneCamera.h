#pragma once
#include <vector>
#include "camera.h"
#include "Entidad.h"

constexpr int NO_CHARACTER = 0;   // el replay usa 0 para "no es un combatiente"

enum class SceneCameraType { Free, FollowBehind, FollowFront, FirstPerson, Duel, WholeMap, Overhead };

const char* const SCENE_CAMERA_TYPE_NAMES[] = {   // en el mismo orden que SceneCameraType
	"Libre", "Espalda", "Frontal", "Primera persona", "Duelo", "Mapa completo", "Cenital"
};

inline const char* sceneCameraTypeName(SceneCameraType type) { return SCENE_CAMERA_TYPE_NAMES[(int)type]; }

struct SceneCamera {   // cada tipo usa solo los campos que le tocan
	SceneCameraType type = SceneCameraType::Free;
	int   targetCharacterId = NO_CHARACTER;
	float distance = 0.f;      // segun el tipo: detras, delante, margen del duelo o alejamiento del mapa
	float height = 0.f;        // sobre los pies del personaje, o sobre el suelo
	float sideOffset = 0.f;    // + a la derecha del personaje: asi no tapa el centro de la imagen
	float fieldOfViewDegrees = 45.f;
	CameraPose freePose{ glm::vec3(0.f, 20.f, 60.f), -90.f, -20.f };   // solo para el tipo libre
};

bool needsCharacter(SceneCameraType type);   // los tipos que siguen a un personaje

SceneCamera makeSceneCamera(SceneCameraType type, int targetCharacterId);   // con los valores de partida de su tipo
glm::mat4 viewMatrixFor(const SceneCamera& camera, const std::vector<Entidad>& frameEntities);
std::vector<int> characterIdsIn(const std::vector<Entidad>& frameEntities);

struct CameraView {   // lo que necesita una vista para pintarse
	glm::mat4 view;
	float fieldOfViewDegrees;
};

CameraView cameraViewFor(const SceneCamera& camera, const std::vector<Entidad>& frameEntities);
const Entidad* findCharacter(const std::vector<Entidad>& frameEntities, int characterId);   // nullptr si no esta en el frame
glm::vec3 chestOf(const Entidad& character);
glm::vec3 combatCenter(const std::vector<Entidad>& frameEntities);   // el centro de la arena si no hay combatientes