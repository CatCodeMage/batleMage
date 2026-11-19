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

bool CollisionManager::checkCollision(
	const SphereCollider& a,
	const SphereCollider& b)
{
	float dx = a.center.x - b.center.x;
	float dy = a.center.y - b.center.y;
	float dz = a.center.z - b.center.z;

	float radiosum = a.radius + b.radius;

	if (dx * dx + dy * dy + dz * dz
		<= radiosum * radiosum)
	{
		return true;
	}

	return false;
}

CollisionInfo CollisionManager::getGroundCollisionInfo(
	const SphereCollider& sphere,
	float groundHeight)
{
	CollisionInfo info;

	float bottom =
		sphere.center.y - sphere.radius;

	if (bottom > groundHeight)
	{
		return info;
	}

	info.collision = true;

	// El suelo empuja hacia arriba
	info.normal =
		glm::vec3(0.0f, 1.0f, 0.0f);

	// Cuanto se ha metido la esfera en el suelo
	info.penetration =
		groundHeight - bottom;

	return info;
}

CollisionInfo CollisionManager::getCollisionInfo(
    const BoxCollider& box,
    const SphereCollider& sphere)
{
    CollisionInfo info;

    glm::vec3 minPoint =
        box.center - box.halfSize;

    glm::vec3 maxPoint =
        box.center + box.halfSize;

    glm::vec3 closestPoint =
        glm::clamp(
            sphere.center,
            minPoint,
            maxPoint
        );

    glm::vec3 delta =
        sphere.center - closestPoint;

    float distanceSquared =
        glm::dot(delta, delta);

    float radiusSquared =
        sphere.radius * sphere.radius;

    if (distanceSquared > radiusSquared)
    {
        return info;
    }

    info.collision = true;

    float distance =
        std::sqrt(distanceSquared);

    // Caso normal: el centro de la esfera
    // esta fuera de la caja
    if (distance > 0.0001f)
    {
        info.normal =
            delta / distance;

        info.penetration =
            sphere.radius - distance;

        return info;
    }

    // Caso especial:
    // el centro de la esfera esta dentro de la caja

    float left =
        sphere.center.x - minPoint.x;

    float right =
        maxPoint.x - sphere.center.x;

    float bottom =
        sphere.center.y - minPoint.y;

    float top =
        maxPoint.y - sphere.center.y;

    float back =
        sphere.center.z - minPoint.z;

    float front =
        maxPoint.z - sphere.center.z;

    float minDistance = left;
    info.normal =
        glm::vec3(-1.0f, 0.0f, 0.0f);

    if (right < minDistance)
    {
        minDistance = right;
        info.normal =
            glm::vec3(1.0f, 0.0f, 0.0f);
    }

    if (bottom < minDistance)
    {
        minDistance = bottom;
        info.normal =
            glm::vec3(0.0f, -1.0f, 0.0f);
    }

    if (top < minDistance)
    {
        minDistance = top;
        info.normal =
            glm::vec3(0.0f, 1.0f, 0.0f);
    }

    if (back < minDistance)
    {
        minDistance = back;
        info.normal =
            glm::vec3(0.0f, 0.0f, -1.0f);
    }

    if (front < minDistance)
    {
        minDistance = front;
        info.normal =
            glm::vec3(0.0f, 0.0f, 1.0f);
    }

    info.penetration =
        sphere.radius + minDistance;

    return info;
}

