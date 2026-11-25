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
		glm::vec3(0.0f),          // velocity
        radius                    // radius
    };
    rapidezInicial = speed;
    // Inicialización de estado
    isActive = false;
    maxLifeTime = 5.0f;
    currentLifeTime = 0.0f;
}

void Hechizo::cast(const glm::vec3& initialPosition, const glm::vec3& direction) {
    movement.translacio = initialPosition;
    movement.velocity = glm::normalize(direction) * rapidezInicial;
    currentLifeTime = 0.0f;
    isActive = true; // Activamos el proyectil al lanzarlo
}

// calculo movimiento lineal
void Hechizo::moverLineal(float dt) {
    movement.translacio += movement.velocity * dt;
}

// Caida con gravedad: x = x0 + v*dt + 1/2*g*dt^2 calculo parabolico
void Hechizo::moverParabolico(float dt) {
    const glm::vec3 gravedad(0.0f, -SPELL_GRAVITY, 0.0f);

    movement.translacio += movement.velocity * dt + 0.5f * gravedad * dt * dt;
    movement.velocity += gravedad * dt;
}


// Direccion actual: la velocidad normalizada.
// Si esta parado devuelve un vector nulo, para no dividir entre 0 (NaN)
glm::vec3 Hechizo::getDirection() const {
    const float v = glm::length(movement.velocity);
    if (v < 0.0001f) return glm::vec3(0.0f);
    return movement.velocity / v;
}


// CAMBIAR: segun el tipo de movimiento y tipo de casteo, de momento como un proectil
// (sin cambios: el rebote ya no se comprueba aquí, lo avisa colisiones)
void Hechizo::update(float deltaTime) {
    if (!isActive) return;

    switch (hechizo.formaMovimiento) {
        case MovementPattern::Linear:    moverLineal(deltaTime);    break;
        case MovementPattern::Parabolic: moverParabolico(deltaTime); break;
        //case MovementPattern::Homing:    moverTeledirigido(deltaTime); break;
        //case MovementPattern::Orbiting:  moverOrbital(deltaTime);   break;
        case MovementPattern::Static:    break;
    }

    // Acumular tiempo
    currentLifeTime += deltaTime;

    //Comprobar si ha terminado su vida
    if (hasExpired()) {
        setActive(false);
    }
}

// NUEVO: reacción a un choque con el mundo (lo detecta colisiones, no el hechizo)
void Hechizo::alChocarSuperficie(const glm::vec3& normal, float penetracion) {
    if (!isActive) return;

    // Si ya se está alejando de la superficie (p. ej. acaba de rebotar), no hacer nada
    if (glm::dot(movement.velocity, normal) >= 0.0f) return;

    // Sin etiqueta de rebote o sin rebotes restantes: choca y desaparece
    if (!(etiquetas & ETQ_REBOTE) || rebotesRestantes <= 0) {
        setActive(false);
        return;
    }

    // Rebote 3D: refleja la dirección respecto a la normal (sirve para suelo, paredes, rocas...)
    movement.velocity = glm::reflect(movement.velocity, normal);
    // Sacarlo de la superficie para que no siga "dentro"
    movement.translacio += normal * penetracion;
    rebotesRestantes--;
}

void Hechizo::reset() {
    // Reiniciamos el movimiento a valores nulos/ceros
    movement.translacio = glm::vec3(0.0f);
    movement.rotacio = glm::vec3(0.0f);
    movement.scale = glm::vec3(1.0f); // O 0.0f si prefieres que desaparezca por completo
    movement.radius = 0.0f;
    movement.velocity = glm::vec3(0.0f);
    // Reiniciamos tiempos y desactivamos el hechizo
    currentLifeTime = 0.0f;
    isActive = false;

    // NUEVO: limpiamos comportamientos y lanzador (indicePool NO se toca)
    idLanzador = -1;
    idEntidad = INVALID_ENTITY;   // el id era de ese lanzamiento: al volver al pool deja de valer
    etiquetas = ETQ_NINGUNA;
    rebotesRestantes = 0;
}

void Hechizo::setDefinition(SpellDefinitionComponent hechizoDatos) {
    hechizo = hechizoDatos;
}