#include "Collision.h"
#include <algorithm>
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

static float distSqPointToSegment2D(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b)
{
    const glm::vec2 ab = b - a;
    const float lenSq = glm::dot(ab, ab);
    if (lenSq <= 1e-12f)
    {
        const glm::vec2 pa = p - a;
        return glm::dot(pa, pa);
    }
    const float t = glm::clamp(glm::dot(p - a, ab) / lenSq, 0.0f, 1.0f);
    const glm::vec2 proj = a + ab * t;
    const glm::vec2 diff = p - proj;
    return glm::dot(diff, diff);
}

bool CollisionManager::checkCollision(
    const BoxZoneCollider& box,
    const CylinderCollider& cylinder)
{
    if (box.width < 0.0f || box.height < 0.0f || box.length < 0.0f ||
        cylinder.radius < 0.0f || cylinder.halfHeight < 0.0f)
    {
        return false;
    }

    // 1. Solapamiento vertical en Y (franja centrada en box.origin.y)
    const float dy = std::abs(cylinder.center.y - box.origin.y);
    if (dy > (box.height * 0.5f + cylinder.halfHeight))
    {
        return false;
    }

    // 2. Direccion horizontal normalizada en XZ
    const glm::vec2 dirXZ(box.direction.x, box.direction.z);
    const float dirLenSq = glm::dot(dirXZ, dirXZ);
    if (dirLenSq <= 1e-12f)
    {
        return false;
    }
    const glm::vec2 fwd = dirXZ / std::sqrt(dirLenSq);

    // 3. Vector perpendicular horizontal
    const glm::vec2 right(-fwd.y, fwd.x);

    // 4. Transformacion a coordenadas locales de la caja mediante productos escalares
    const glm::vec2 rel(cylinder.center.x - box.origin.x, cylinder.center.z - box.origin.z);
    const float localX = glm::dot(rel, right);
    const float localZ = glm::dot(rel, fwd);

    // 5. Punto del rectangulo [-width/2, width/2] x [0, length] mas cercano al cilindro
    const float halfW = box.width * 0.5f;
    const float clampedX = glm::clamp(localX, -halfW, halfW);
    const float clampedZ = glm::clamp(localZ, 0.0f, box.length);

    // 6. Comparar distancia al cuadrado con radio al cuadrado
    const float dx = localX - clampedX;
    const float dz = localZ - clampedZ;
    return (dx * dx + dz * dz) <= (cylinder.radius * cylinder.radius);
}

bool CollisionManager::checkCollision(
    const ConeCollider& cone,
    const CylinderCollider& cylinder)
{
    if (cone.range <= 0.0f || cone.height < 0.0f || cone.angle <= 0.0f ||
        cylinder.radius < 0.0f || cylinder.halfHeight < 0.0f)
    {
        return false;
    }

    // 1. Solapamiento vertical en Y (franja centrada en cone.origin.y)
    const float dy = std::abs(cylinder.center.y - cone.origin.y);
    if (dy > (cone.height * 0.5f + cylinder.halfHeight))
    {
        return false;
    }

    // 2. Direccion horizontal normalizada en XZ
    const glm::vec2 dirXZ(cone.direction.x, cone.direction.z);
    const float dirLenSq = glm::dot(dirXZ, dirXZ);
    if (dirLenSq <= 1e-12f)
    {
        return false;
    }
    const glm::vec2 fwd = dirXZ / std::sqrt(dirLenSq);

    // 3. Posicion relativa del cilindro en XZ
    const glm::vec2 rel(cylinder.center.x - cone.origin.x, cylinder.center.z - cone.origin.z);
    const float distSq = glm::dot(rel, rel);

    // 4. Descartar si supera alcance + radio
    const float maxReach = cone.range + cylinder.radius;
    if (distSq > maxReach * maxReach)
    {
        return false;
    }

    // Contacto directo con el vertice/origen del cono (o circulo completo >= 360 deg)
    const float radSq = cylinder.radius * cylinder.radius;
    if (distSq <= radSq || cone.angle >= 360.0f)
    {
        return true;
    }

    // 5. Comprobar si el centro del cilindro esta dentro de la apertura angular
    const float halfAngleRad = glm::radians(cone.angle * 0.5f);
    const float cosHalf = std::cos(halfAngleRad);
    const float dist = std::sqrt(distSq);
    if (glm::dot(rel, fwd) >= dist * cosHalf)
    {
        // 6. Dentro de la apertura y dist <= range + radius -> colision con el sector/limite circular
        return true;
    }

    // 7. Fuera de la apertura: distancia minima a los dos segmentos laterales del sector
    const float sinHalf = std::sin(halfAngleRad);
    const glm::vec2 segLeft(
        (fwd.x * cosHalf - fwd.y * sinHalf) * cone.range,
        (fwd.x * sinHalf + fwd.y * cosHalf) * cone.range);
    const glm::vec2 segRight(
        (fwd.x * cosHalf + fwd.y * sinHalf) * cone.range,
        (-fwd.x * sinHalf + fwd.y * cosHalf) * cone.range);

    const glm::vec2 origin2D(0.0f, 0.0f);
    const float dLeftSq = distSqPointToSegment2D(rel, origin2D, segLeft);
    const float dRightSq = distSqPointToSegment2D(rel, origin2D, segRight);

    // 8. Comparar distancia minima al cuadrado con el radio al cuadrado
    return (std::min)(dLeftSq, dRightSq) <= radSq;
}

