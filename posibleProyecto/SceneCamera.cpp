#include "SceneCamera.h"
#include <cmath>

namespace {
	const glm::vec3 WORLD_UP(0.f, 1.f, 0.f);
	const glm::vec3 ARENA_CENTER(0.f);
	const glm::vec3 SCREEN_UP_SEEN_FROM_ABOVE(0.f, 0.f, -1.f);   // mirando recto hacia abajo, WORLD_UP no sirve de "arriba"
	constexpr float CHEST_HEIGHT = 1.2f;          // a donde se mira en un personaje, sobre sus pies
	constexpr float LOOK_AHEAD_DISTANCE = 4.0f;   // la camara de espalda mira por delante: se ve hacia donde va
	constexpr float FACE_CLEARANCE = 0.3f;        // en primera persona, el ojo va delante de la cara para no ver la cabeza por dentro
	constexpr float MIN_DUEL_SEPARATION = 0.01f;  // por debajo, la linea entre los dos no da una direccion

	const Entidad* findCharacter(const std::vector<Entidad>& entities, int characterId) {
		for (const Entidad& entity : entities)
			if (entity.characterId == characterId) return &entity;
		return nullptr;
	}

	const Entidad* findOpponent(const std::vector<Entidad>& entities, int characterId) {
		for (const Entidad& entity : entities)
			if (entity.characterId != NO_CHARACTER && entity.characterId != characterId) return &entity;
		return nullptr;
	}

	glm::vec3 forwardOf(const Entidad& character) {   // el replay guarda la guinada con 0 mirando a +Z
		const float yaw = character.transform.rot.y;
		return glm::vec3(std::sin(yaw), 0.f, std::cos(yaw));
	}

	glm::vec3 rightOf(const Entidad& character) { return glm::normalize(glm::cross(forwardOf(character), WORLD_UP)); }
	glm::vec3 chestOf(const Entidad& character) { return character.transform.pos + WORLD_UP * CHEST_HEIGHT; }

	glm::vec3 combatCenter(const std::vector<Entidad>& entities) {
		glm::vec3 positionSum(0.f);
		int characterCount = 0;
		for (const Entidad& entity : entities) {
			if (entity.characterId != NO_CHARACTER)  {
				positionSum += entity.transform.pos;
				++characterCount;
			}
		}
		return characterCount > 0 ? (positionSum / (float)characterCount) : ARENA_CENTER;
	}

	glm::mat4 viewOfFreePose(const CameraPose& pose) {
		return glm::lookAt(pose.position, pose.position + frontFromYawPitch(pose.yaw, pose.pitch), WORLD_UP);
	}

	glm::mat4 viewFromBehind(const SceneCamera& camera, const Entidad& character) {
		const glm::vec3 forward = forwardOf(character);
		const glm::vec3 eye = character.transform.pos - forward * camera.distance
			+ WORLD_UP * camera.height + rightOf(character) * camera.sideOffset;
		return glm::lookAt(eye, chestOf(character) + forward * LOOK_AHEAD_DISTANCE, WORLD_UP);
	}

	glm::mat4 viewFromFront(const SceneCamera& camera, const Entidad& character) {
		const glm::vec3 eye = character.transform.pos + forwardOf(character) * camera.distance
			+ WORLD_UP * camera.height + rightOf(character) * camera.sideOffset;
		return glm::lookAt(eye, chestOf(character), WORLD_UP);
	}

	glm::mat4 viewFromEyes(const SceneCamera& camera, const Entidad& character) {
		const glm::vec3 forward = forwardOf(character);
		const glm::vec3 eye = character.transform.pos + WORLD_UP * camera.height + forward * FACE_CLEARANCE;
		return glm::lookAt(eye, eye + forward, WORLD_UP);
	}

	glm::mat4 viewOfDuel(const SceneCamera& camera, const Entidad& character, const Entidad* opponent) {
		if (!opponent) return viewFromBehind(camera, character);   // sin rival en este frame, no hay duelo que encuadrar

		const glm::vec3 between = opponent->transform.pos - character.transform.pos;
		const glm::vec3 midpoint = character.transform.pos + between * 0.5f;
		const float separation = glm::length(between);
		const glm::vec3 side = separation > MIN_DUEL_SEPARATION
			? glm::normalize(glm::cross(between, WORLD_UP))
			: rightOf(character);
		// con el FOV vertical: el horizontal es mas ancho, asi que los dos caben con margen
		const float distanceToFitBoth = separation * 0.5f / std::tan(glm::radians(camera.fieldOfViewDegrees) * 0.5f);
		const glm::vec3 eye = midpoint + side * (distanceToFitBoth + camera.distance) + WORLD_UP * camera.height;
		return glm::lookAt(eye, midpoint + WORLD_UP * CHEST_HEIGHT, WORLD_UP);
	}

	glm::mat4 viewOfWholeMap(const SceneCamera& camera) {
		const glm::vec3 eye = ARENA_CENTER + glm::vec3(0.f, camera.height, camera.distance);
		return glm::lookAt(eye, ARENA_CENTER, WORLD_UP);
	}

	glm::mat4 viewFromAbove(const SceneCamera& camera, const std::vector<Entidad>& entities) {
		const glm::vec3 center = combatCenter(entities);
		return glm::lookAt(center + WORLD_UP * camera.height, center, SCREEN_UP_SEEN_FROM_ABOVE);
	} 
}

SceneCamera makeSceneCamera(SceneCameraType type, int targetCharacterId) {
	SceneCamera camera;
	camera.type = type;
	camera.targetCharacterId = targetCharacterId;
	switch (type) {   // valores de partida: luego se ajustan en el panel
	case SceneCameraType::FollowBehind: camera.distance = 6.f;  camera.height = 3.f;  camera.sideOffset = 1.f; camera.fieldOfViewDegrees = 50.f; break;
	case SceneCameraType::FollowFront:  camera.distance = 5.f;  camera.height = 2.f;  break;
	case SceneCameraType::FirstPerson:  camera.height = 1.7f;   camera.fieldOfViewDegrees = 70.f; break;
	case SceneCameraType::Duel:         camera.distance = 4.f;  camera.height = 5.f;  break;
	case SceneCameraType::WholeMap:     camera.distance = 70.f; camera.height = 50.f; break;
	case SceneCameraType::Overhead:     camera.height = 40.f;   break;
	case SceneCameraType::Free:         break;
	}
	return camera;
}

bool needsCharacter(SceneCameraType type) {
	return type == SceneCameraType::FollowBehind || type == SceneCameraType::FollowFront
		|| type == SceneCameraType::FirstPerson || type == SceneCameraType::Duel;
}

glm::mat4 viewMatrixFor(const SceneCamera& camera, const std::vector<Entidad>& frameEntities) {
	const Entidad* character = findCharacter(frameEntities, camera.targetCharacterId);
	if (needsCharacter(camera.type) && !character) return viewOfWholeMap(camera);   // el personaje no esta en este frame

	switch (camera.type) {
	case SceneCameraType::Free:         return viewOfFreePose(camera.freePose);
	case SceneCameraType::FollowBehind: return viewFromBehind(camera, *character);
	case SceneCameraType::FollowFront:  return viewFromFront(camera, *character);
	case SceneCameraType::FirstPerson:  return viewFromEyes(camera, *character);
	case SceneCameraType::Duel:         return viewOfDuel(camera, *character, findOpponent(frameEntities, camera.targetCharacterId));
	case SceneCameraType::WholeMap:     return viewOfWholeMap(camera);
	case SceneCameraType::Overhead:     return viewFromAbove(camera, frameEntities);
	}
	return viewOfWholeMap(camera);   // no se llega: el switch cubre todos los tipos
}

std::vector<int> characterIdsIn(const std::vector<Entidad>& frameEntities) {
	std::vector<int> characterIds;
	for (const Entidad& entity : frameEntities)
		if (entity.characterId != NO_CHARACTER) characterIds.push_back(entity.characterId);
	return characterIds;
}