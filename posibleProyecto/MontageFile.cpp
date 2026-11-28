#include "MontageFile.h"
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
	json vectorToJson(const glm::vec3& vector) { return { vector.x, vector.y, vector.z }; }

	glm::vec3 vectorFromJson(const json& values) {
		return { values.at(0).get<float>(), values.at(1).get<float>(), values.at(2).get<float>() };
	}

	// los enums se guardan como numero: uno fuera de rango haria leer fuera de su array de nombres
	int enumIndexFromJson(const json& values, const char* key, int valueCount) {
		const int index = values.at(key).get<int>();
		const bool isKnownValue = index >= 0 && index < valueCount;
		if (!isKnownValue)
			throw std::runtime_error(std::string(key) + " = " + std::to_string(index) + ", y solo hay " + std::to_string(valueCount) + " valores");
		return index;
	}

	json cameraToJson(const SceneCamera& camera) {
		return {
			{ "cameraType", (int)camera.type },
			{ "targetCharacterId", camera.targetCharacterId },
			{ "distance", camera.distance },
			{ "height", camera.height },
			{ "sideOffset", camera.sideOffset },
			{ "fieldOfViewDegrees", camera.fieldOfViewDegrees },
			{ "freePosition", vectorToJson(camera.freePose.position) },
			{ "freeYaw", camera.freePose.yaw },
			{ "freePitch", camera.freePose.pitch },
		};
	}

	SceneCamera cameraFromJson(const json& values) {
		SceneCamera camera;
		camera.type = (SceneCameraType)enumIndexFromJson(values, "cameraType", (int)std::size(SCENE_CAMERA_TYPE_NAMES));
		camera.targetCharacterId = values.at("targetCharacterId").get<int>();
		camera.distance = values.at("distance").get<float>();
		camera.height = values.at("height").get<float>();
		camera.sideOffset = values.at("sideOffset").get<float>();
		camera.fieldOfViewDegrees = values.at("fieldOfViewDegrees").get<float>();
		camera.freePose = { vectorFromJson(values.at("freePosition")),
			values.at("freeYaw").get<float>(), values.at("freePitch").get<float>() };
		return camera;
	}

	json transitionToJson(const ShotTransition& transition) {
		return {
			{ "transitionType", (int)transition.type },
			{ "durationFrames", transition.durationFrames },
			{ "focus", (int)transition.focus },
			{ "focusCharacterId", transition.focusCharacterId },
			{ "arcHeight", transition.arcHeight },
		};
	}

	ShotTransition transitionFromJson(const json& values) {
		ShotTransition transition;
		transition.type = (TransitionType)enumIndexFromJson(values, "transitionType", (int)std::size(TRANSITION_TYPE_NAMES));
		transition.durationFrames = values.at("durationFrames").get<int>();
		transition.focus = (CameraMoveFocus)enumIndexFromJson(values, "focus", (int)std::size(CAMERA_MOVE_FOCUS_NAMES));
		transition.focusCharacterId = values.at("focusCharacterId").get<int>();
		transition.arcHeight = values.at("arcHeight").get<float>();
		return transition;
	}

	json shotToJson(const CameraShot& shot) {
		return {
			{ "startFrame", shot.startFrame },
			{ "camera", cameraToJson(shot.camera) },
			{ "transition", transitionToJson(shot.transition) },
		};
	}

	CameraShot shotFromJson(const json& values) {
		return { values.at("startFrame").get<int>(), cameraFromJson(values.at("camera")), transitionFromJson(values.at("transition")) };
	}

	// viewAtFrame da por hecho que hay un plano en el frame 0 y que van en orden
	void checkShotsAreUsable(const std::vector<CameraShot>& shots) {
		if (shots.empty()) throw std::runtime_error("no hay planos");
		if (shots.front().startFrame != 0)
			throw std::runtime_error("el primer plano empieza en el frame " + std::to_string(shots.front().startFrame) + ", no en el 0");
		for (size_t shotIndex = 1; shotIndex < shots.size(); ++shotIndex) {
			const bool isAfterPrevious = shots[shotIndex].startFrame > shots[shotIndex - 1].startFrame;
			if (!isAfterPrevious)
				throw std::runtime_error("el plano " + std::to_string(shotIndex) + " (frame " + std::to_string(shots[shotIndex].startFrame)
					+ ") no va despues del anterior (frame " + std::to_string(shots[shotIndex - 1].startFrame) + ")");
		}
	}
}

void saveMontage(const std::filesystem::path& file, const std::vector<CameraShot>& shots) {
	json shotList = json::array();
	for (const CameraShot& shot : shots) shotList.push_back(shotToJson(shot));

	std::ofstream output(file);
	if (!output) throw std::runtime_error("no se puede escribir " + file.string());
	output << json{ { "shots", shotList } }.dump(4);   // con sangria: se puede leer para asi mostarse mas bonito en el documento 
}

std::vector<CameraShot> loadMontage(const std::filesystem::path& file) {
	std::ifstream input(file);
	if (!input) throw std::runtime_error("no se puede abrir " + file.string());
	try {
		const json document = json::parse(input);
		std::vector<CameraShot> shots;
		for (const json& shotValues : document.at("shots")) shots.push_back(shotFromJson(shotValues));
		checkShotsAreUsable(shots);
		return shots;
	}
	catch (const std::exception& error) {
		throw std::runtime_error(file.string() + ": " + error.what());   // la libreria dice que clave falla; esto anade en que fichero
	}
}