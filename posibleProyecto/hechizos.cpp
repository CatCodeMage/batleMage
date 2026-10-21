#include "hechizos.h"

Hechizo::Hechizo(int id, const std::string& name, ElementType element, 
                             float cost, float damage, float cooldown, 
                             float speed, float radius) 
{
    // Inicialización del componente de definición
    //CAMBIAR: en el futuro meter casttype y movementpattern
    hechizo = { id, name, element, CastType::Projectile, MovementPattern::Linear, cost, damage, cooldown };
    
    // Inicialización del componente de movimiento (valores por defecto para posición/rotación)
    movement = { 
        glm::vec3(0.0f),          // translacio
        glm::vec3(0.0f),          // rotacio
        glm::vec3(1.0f),          // scale
        glm::vec3(0.0f, 0.0f, 1.0f), // direction (por defecto hacia adelante en Z)
        speed,                    // speed
        radius                    // radius
    };

    // Inicialización de estado
    isActive = false;
    maxLifeTime = 5.0f;
    currentLifeTime = 0.0f;
}

void Hechizo::cast(const glm::vec3& initialPosition, const glm::vec3& direction) {
    movement.translacio = initialPosition;
    movement.direction = glm::normalize(direction);
    currentLifeTime = 0.0f;
    isActive = true; // Activamos el proyectil al lanzarlo
}

// CAMBIAR: segun el tipo de movimiento y tipo de casteo, de momento como un proectil
void Hechizo::update(float deltaTime) {
    if (!isActive) return;

    // Actualizar la posición (translacio) en base a la velocidad y dirección
    movement.translacio += movement.direction * (movement.speed * deltaTime);

    // Acumular tiempo
    currentLifeTime += deltaTime;

    //Comprobar si ha terminado su vida
    if (hasExpired()) {
        setActive(false); 
    }
}

void Hechizo::reset() {
    // Reiniciamos el movimiento a valores nulos/ceros
    movement.translacio = glm::vec3(0.0f);
    movement.rotacio = glm::vec3(0.0f);
    movement.scale = glm::vec3(1.0f); // O 0.0f si prefieres que desaparezca por completo
    movement.direction = glm::vec3(0.0f, 0.0f, 0.0f); // Sin dirección
    movement.speed = 0.0f;
    movement.radius = 0.0f;

    // Reiniciamos tiempos y desactivamos el hechizo
    currentLifeTime = 0.0f;
    isActive = false; 
}

void Hechizo::setDefinition(SpellDefinitionComponent hechizoDatos){
    hechizo = hechizoDatos;
}
