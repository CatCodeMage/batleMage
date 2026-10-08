#pragma once
#include <glm/glm.hpp>

struct CylinderCollider
{
	glm::vec3 center;
	float radius;
	float halfHeight;
};

struct SphereCollider
{
	glm::vec3 center;
	float radius;
};

struct BoxCollider
{
	glm::vec3 center;
	glm::vec3 halfSize;
};


class CollisionManager {
public:
	bool checkCollision(const CylinderCollider& a, const CylinderCollider& b);
	bool checkCollision(const SphereCollider& esfera,const CylinderCollider& cilindre);
	bool checkCollision(const BoxCollider& cub, const SphereCollider& esfera);
	//Gestionar el daño y sus tipos a personajes
};
