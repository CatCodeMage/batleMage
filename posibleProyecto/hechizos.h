#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <cmath>
#include <memory>
#include "hechizoDatos.h"


class Hechizo {
public:

    Hechizo(int id, const std::string& name, ElementType element, float cost,
        float damage, float cooldown, float speed, float radius);

    //***************************** */
    // Funciones (lógica de hechizos)
    //***************************** */

    // Lanza el hechizo inicializando su posición y dirección
    void cast(const glm::vec3& initialPosition, const glm::vec3& direction);

    // Actualiza la posición y comprueba si debe expirar
    void update(float deltaTime);

    // Función para reiniciar el proyectil existente
    void reset();

    // Función para saber si ha expirado tiempo de vida
    bool hasExpired() const {
        return currentLifeTime >= maxLifeTime;
    }

    //***************************** */
    // GETTERS (Consultas de datos)
    //***************************** */

    // Estado general
    bool IsActive() const { return isActive; }
    float getMaxLifeTime() const { return maxLifeTime; }
    float getCurrentLifeTime() const { return currentLifeTime; }

    // Datos del hechizo (Definición)
    int getId() const { return hechizo.id; }
    const std::string& getName() const { return hechizo.name; }
    ElementType getElement() const { return hechizo.element; }
    float getCost() const { return hechizo.cost; }
    float getDamage() const { return hechizo.damage; }
    float getCooldown() const { return hechizo.cooldown; }

    // Datos de movimiento y físicas del proyectil
    const glm::vec3& getPosition() const { return movement.translacio; }
    const glm::vec3& getRotation() const { return movement.rotacio; }
    const glm::vec3& getScale() const { return movement.scale; }
    const glm::vec3& getDirection() const { return movement.direction; }
    float getSpeed() const { return movement.speed; }
    float getRadius() const { return movement.radius; }

    //***************************** */
    // SETTERS (Modificación de datos)
    //***************************** */

    // Modificar datos del hechizo en caliente
    void setDamage(float newDamage) { hechizo.damage = newDamage; }
    void setCost(float newCost) { hechizo.cost = newCost; }
    void setCooldown(float newCooldown) { hechizo.cooldown = newCooldown; }

    // Modificar datos de movimiento/físicas
    void setPosition(const glm::vec3& newPos) { movement.translacio = newPos; }
    void setRotation(const glm::vec3& newRot) { movement.rotacio = newRot; }
    void setScale(const glm::vec3& newScale) { movement.scale = newScale; }
    void setSpeed(float newSpeed) { movement.speed = newSpeed; }
    void setRadius(float newRadius) { movement.radius = newRadius; }

    // Control manual de ciclo de vida si fuera necesario
    void setActive(bool active) { isActive = active; }
    void setMaxLifeTime(float time) { maxLifeTime = time; }

    // hechizo entero
    void setDefinition(SpellDefinitionComponent hechizoDatos);

    // definición completa del hechizo en vuelo (con modificadores ya aplicados)
    const SpellDefinitionComponent& getDefinition() const { return hechizo; }

    void setDirection(const glm::vec3& newDir) { movement.direction = newDir; }

    // quién lo ha lanzado (-1 = desconocido)
    void setLanzador(int id) { idLanzador = id; }
    int  getLanzador() const { return idLanzador; }

    // comportamientos activados por modificadores (sin memoria dinámica)
    void setComportamientos(uint32_t etq, int rebotes) {
        etiquetas = etq;
        rebotesRestantes = rebotes;
    }
    uint32_t getEtiquetas() const { return etiquetas; }
    int getRebotesRestantes() const { return rebotesRestantes; }

    // colisiones avisa de que el hechizo ha chocado con una superficie del mundo.
    // vector normal (perpendicular a la superficie): apunta desde la superficie HACIA el hechizo. penetracion: cuánto se ha metido (>= 0).
    // El hechizo decide qué hacer: rebotar o desactivarse.
    void alChocarSuperficie(const glm::vec3& normal, float penetracion);

    // posición dentro del pool (la gestiona HechizosGestor)
    void setIndicePool(int indice) { indicePool = indice; }
    int  getIndicePool() const { return indicePool; }

private:
    // struct con los datos del hechizo
    SpellDefinitionComponent hechizo;

    // struct con los datos de movimiento de projectil
    ProjectileComponent movement;

    //variables para saber el tiempo de vida
    bool isActive;
    float maxLifeTime;
    float currentLifeTime;

    // NUEVO: datos de comportamientos y control
    int      idLanzador = -1;
    uint32_t etiquetas = ETQ_NINGUNA;
    int      rebotesRestantes = 0;
    int      indicePool = -1;   // NO se borra en reset()
};