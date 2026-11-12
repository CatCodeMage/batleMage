
#include "hechizos.h"

Hechizo::Hechizo(int id, const std::string& name, ElementType element,
    float cost, float damage, float cooldown,
    float speed, float radius)
{
    // Definicion del hechizo
    hechizo = {
        id, name, element,
        CastType::Projectile,
        MovementPattern::Linear,
        cost, damage, cooldown
    };

    // Estado inicial del proyectil
    movement = {
        glm::vec3(0.0f),          // translacio
        glm::vec3(0.0f),          // rotacio
        glm::vec3(1.0f),          // scale
        glm::vec3(0.0f, 0.0f, 1.0f), // direction
        glm::vec3(0.0f),          // velocity (NUEVO)
        speed,
        radius
    };

    isActive = false;
    maxLifeTime = 5.0f;
    currentLifeTime = 0.0f;
}


void Hechizo::cast(const glm::vec3& initialPosition,
    const glm::vec3& direction)
{
    movement.translacio = initialPosition;
    movement.direction = glm::normalize(direction);

    // Velocidad inicial del proyectil
    movement.velocity = movement.direction * movement.speed;

    if (hechizo.formaMovimiento == MovementPattern::Parabolic) {
        // Sale horizontalmente, sin impulso hacia arriba
        movement.velocity.y = 0.0f;
    }

    currentLifeTime = 0.0f;
    isActive = true;
}


void Hechizo::update(float deltaTime)
{
    if (!isActive) return;

    if (hechizo.formaMovimiento == MovementPattern::Parabolic) {

        // Gravedad negativa porque el eje Y apunta hacia arriba
        const glm::vec3 gravedad(0.0f, -1.5f, 0.0f);

        // Actualizar posicion teniendo en cuenta la aceleracion
        movement.translacio +=
            movement.velocity * deltaTime +
            0.5f * gravedad * deltaTime * deltaTime;

        // La gravedad modifica la velocidad vertical
        movement.velocity += gravedad * deltaTime;
    }
    else {
        // Movimiento lineal original
        movement.translacio +=
            movement.direction * (movement.speed * deltaTime);
    }

    currentLifeTime += deltaTime;

    if (hasExpired()) {
        setActive(false);
    }
}

void Hechizo::reset()
{
    movement.translacio = glm::vec3(0.0f);
    movement.rotacio = glm::vec3(0.0f);
    movement.scale = glm::vec3(1.0f);
    movement.direction = glm::vec3(0.0f);
    movement.velocity = glm::vec3(0.0f);

    movement.speed = 0.0f;
    movement.radius = 0.0f;

    currentLifeTime = 0.0f;
    isActive = false;
}

void Hechizo::setDefinition(SpellDefinitionComponent hechizoDatos)
{
    hechizo = hechizoDatos;
}
