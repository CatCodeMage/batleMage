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

struct BoxZoneCollider
{
    glm::vec3 origin;
    glm::vec3 direction;
    float width;
    float height;
    float length;
};

struct ConeCollider
{
    glm::vec3 origin;
    glm::vec3 direction;
    float range;
    float angle;
    float height;
};

// Informacion adicional de una colision
struct CollisionInfo
{
    bool collision = false;
    glm::vec3 normal = glm::vec3(0.0f);
    float penetration = 0.0f;
};

class CollisionManager
{
public:
    bool checkCollision(
        const CylinderCollider& a,
        const CylinderCollider& b);

    bool checkCollision(
        const SphereCollider& esfera,
        const CylinderCollider& cilindre);

    bool checkCollision(
        const BoxCollider& cub,
        const SphereCollider& esfera);

    // Esta estaba en tu rama hitbox
    bool checkCollision(
        const SphereCollider& a,
        const SphereCollider& b);

    // Devuelve normal y penetracion contra un objeto tipo caja
    CollisionInfo getCollisionInfo(
        const BoxCollider& box,
        const SphereCollider& sphere);

    // Devuelve normal y penetracion contra el suelo
    CollisionInfo getGroundCollisionInfo(
        const SphereCollider& sphere,
        float groundHeight);

    bool checkCollision(
        const BoxZoneCollider& box,
        const CylinderCollider& cylinder);

    bool checkCollision(
        const ConeCollider& cone,
        const CylinderCollider& cylinder);
};