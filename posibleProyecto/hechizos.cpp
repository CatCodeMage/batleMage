
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
        glm::vec3(0.0f),          // velocity
        radius                    // radius
    };
    rapidezInicial = speed;
    isActive = false;
    maxLifeTime = 5.0f;
    currentLifeTime = 0.0f;
}


void Hechizo::cast(const glm::vec3& initialPosition,
    const glm::vec3& direction)
{
    movement.translacio = initialPosition;
    const float length = glm::length(direction);
    movement.velocity = length > 0.0f ? direction * (rapidezInicial / length) : glm::vec3(0.0f);
    currentLifeTime = 0.0f;
    isActive = true;
}

// calculo movimiento lineal
void Hechizo::moverLineal(float dt) {
    movement.translacio += movement.velocity * dt;
}

// Caida con gravedad: x = x0 + v*dt + 1/2*g*dt^2 calculo parabolico
void Hechizo::moverParabolico(float dt) {
    const float effectiveGravity = SPELL_GRAVITY * hechizo.gravityMultiplier;
    const glm::vec3 gravedad(0.0f, -effectiveGravity, 0.0f);

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
        case MovementPattern::Parabolic: moverParabolico(deltaTime); break;
        case MovementPattern::Static:    break;
        default:                         moverLineal(deltaTime);   break;
    }

    currentLifeTime += deltaTime;

    if (hasExpired()) {
        setActive(false);
    }
}

bool calcularDireccionBalistica(
    const glm::vec3& origin,
    const glm::vec3& target,
    float speed,
    float effectiveGravity,
    float maxLifetime,
    glm::vec3& outDirection)
{
    if (!std::isfinite(speed) || !std::isfinite(effectiveGravity) || !std::isfinite(maxLifetime) ||
        speed <= 0.0001f || maxLifetime <= 0.0f)
    {
        return false;
    }

    const glm::vec3 delta = target - origin;
    const glm::vec2 horiz(delta.x, delta.z);
    const float d = glm::length(horiz);
    const float h = delta.y;

    if (effectiveGravity <= 1e-6f)
    {
        const float dist3D = glm::length(delta);
        if (dist3D <= 1e-6f) return false;
        if (dist3D / speed > maxLifetime) return false;
        outDirection = delta / dist3D;
        return true;
    }

    if (d <= 1e-6f)
    {
        return false;
    }

    const float v2 = speed * speed;
    const float v4 = v2 * v2;
    const float g = effectiveGravity;
    const float disc = v4 - g * (g * d * d + 2.0f * h * v2);
    if (disc < 0.0f || !std::isfinite(disc))
    {
        return false;
    }

    // Solucion de menor angulo para trayectoria directa
    const float tanTheta = (v2 - std::sqrt(disc)) / (g * d);
    const float cosTheta = 1.0f / std::sqrt(1.0f + tanTheta * tanTheta);
    const float sinTheta = tanTheta * cosTheta;

    const float vx = speed * cosTheta;
    if (vx <= 1e-6f) return false;
    const float flightTime = d / vx;
    if (flightTime > maxLifetime)
    {
        return false;
    }

    const glm::vec2 dirXZ = horiz / d;
    outDirection = glm::normalize(glm::vec3(dirXZ.x * cosTheta, sinTheta, dirXZ.y * cosTheta));
    return true;
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

    // Reflejar la velocidad conserva la trayectoria tras el rebote, tambien con gravedad.
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
    rapidezInicial = 0.0f;
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