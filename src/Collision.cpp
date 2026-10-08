#include "Collision.h"
#include <cmath>

bool CollisionManager::checkCollision(const CylinderCollider& a, const CylinderCollider& b) {
	float dx = a.center.x - b.center.x;
	float dz = a.center.z - b.center.z;
	float radiosum = a.radius + b.radius;

	if (dx * dx + dz * dz <= radiosum * radiosum) {
		float dy = std::abs(a.center.y - b.center.y);
		if (dy <= a.halfHeight + b.halfHeight)
			return true;
	}
	return false;
}

bool CollisionManager::checkCollision(const SphereCollider& esfera, const CylinderCollider& cilindre) {
	float dx = esfera.center.x - cilindre.center.x;
	float dz = esfera.center.z - cilindre.center.z;
	float radiosum = esfera.radius + cilindre.radius;

	if (dx * dx + dz * dz <= radiosum * radiosum) {
		float dy = std::abs(esfera.center.y - cilindre.center.y);
		if (dy <= esfera.radius + cilindre.halfHeight)
			return true;
	}
	return false;
}

bool CollisionManager::checkCollision(const BoxCollider& cub, const SphereCollider& esfera) {
	//glm::clamp(valor que li pasem, minim, maxim)
	float xMesProper = glm::clamp(esfera.center.x, cub.center.x - cub.halfSize.x, cub.center.x + cub.halfSize.x);
	float yMesProper = glm::clamp(esfera.center.y, cub.center.y - cub.halfSize.y, cub.center.y + cub.halfSize.y);
	float zMesProper = glm::clamp(esfera.center.z, cub.center.z - cub.halfSize.z, cub.center.z + cub.halfSize.z);

	float dx = esfera.center.x - xMesProper;
	float dy = esfera.center.y - yMesProper;
	float dz = esfera.center.z - zMesProper;

	if (dx * dx + dy * dy + dz * dz <= esfera.radius * esfera.radius)
		return true;

	return false;
}

