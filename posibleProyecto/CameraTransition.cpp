#include "CameraTransition.h"
#include <cmath>
#include <optional>
#include <glm/gtc/constants.hpp>

namespace {
	const glm::vec3 WORLD_UP(0.f, 1.f, 0.f);
	constexpr float MIN_DIRECTION_LENGTH = 0.001f;   // por debajo, una direccion no se puede normalizar

	struct CameraPlacement {
		glm::vec3 eye;
		glm::vec3 forward;
		glm::vec3 up;
	};

	CameraPlacement placementOf(const glm::mat4& view) {
		const glm::mat4 cameraToWorld = glm::inverse(view);
		// en OpenGL la camara mira hacia su -Z y su "arriba" es su +Y
		return { glm::vec3(cameraToWorld[3]), -glm::vec3(cameraToWorld[2]), glm::vec3(cameraToWorld[1]) };
	}

	float easeInOut(float progress) { return progress * progress * (3.f - 2.f * progress); }   // arranca y frena suave

	float midwayWeight(float progress) {   // 0 en los extremos y 1 a mitad, sin tirones al empezar ni al acabar
		const float wave = std::sin(glm::pi<float>() * progress);
		return wave * wave;
	}

	glm::vec3 blendDirections(const glm::vec3& from, const glm::vec3& to, float weight) {
		const glm::vec3 mixed = glm::mix(from, to, weight);
		return glm::length(mixed) > MIN_DIRECTION_LENGTH ? glm::normalize(mixed) : to;   // dos direcciones opuestas se anulan a mitad
	}

	std::optional<glm::vec3> focusPointOf(const ShotTransition& transition, const std::vector<Entidad>& frameEntities) {
		switch (transition.focus) {
		case CameraMoveFocus::Natural:      return std::nullopt;
		case CameraMoveFocus::CombatCenter: return combatCenter(frameEntities);
		case CameraMoveFocus::Character: {
			const Entidad* character = findCharacter(frameEntities, transition.focusCharacterId);
			if (!character) return std::nullopt;   // no esta en este frame: se mira como en Natural
			return chestOf(*character);
		}
		}
		return std::nullopt;
	}
}

CameraView blendCameraMove(const CameraView& from, const CameraView& to, float progress,
	const ShotTransition& transition, const std::vector<Entidad>& frameEntities) {
	const CameraPlacement start = placementOf(from.view);
	const CameraPlacement end = placementOf(to.view);
	const float eased = easeInOut(progress);

	const glm::vec3 eye = glm::mix(start.eye, end.eye, eased) + WORLD_UP * (transition.arcHeight * midwayWeight(progress));

	glm::vec3 forward = blendDirections(start.forward, end.forward, eased);
	const std::optional<glm::vec3> focusPoint = focusPointOf(transition, frameEntities);
	if (focusPoint) {
		const glm::vec3 towardFocus = *focusPoint - eye;
		if (glm::length(towardFocus) > MIN_DIRECTION_LENGTH)   // en los extremos manda cada plano; a mitad, el foco
			forward = blendDirections(forward, glm::normalize(towardFocus), midwayWeight(progress));
	}

	const glm::vec3 up = blendDirections(start.up, end.up, eased);   // la cenital no usa +Y como "arriba": con WORLD_UP se daria la vuelta
	const float fieldOfViewDegrees = glm::mix(from.fieldOfViewDegrees, to.fieldOfViewDegrees, eased);
	return { glm::lookAt(eye, eye + forward, up), fieldOfViewDegrees };
}