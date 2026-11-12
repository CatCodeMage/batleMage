
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <memory>

#include "global.h"
#include "CombatTypes.h"

// Tipos de lanzamiento
enum class CastType {
    Projectile,
    AreaOfEffect,
    Cone,
    Raycast,
    Self
};

// Tipos de movimiento
enum class MovementPattern {
    Linear,
    Homing,
    Parabolic,
    Orbiting,
    Static
};

// Propiedades estaticas del hechizo
struct SpellDefinitionComponent {
    int id;
    std::string name;
    ElementType element;
    CastType casteo;
    MovementPattern formaMovimiento;
    float cost;
    float damage;
    float cooldown;
};

// Estado dinamico del proyectil
struct ProjectileComponent {
    glm::vec3 translacio;
    glm::vec3 rotacio;
    glm::vec3 scale;
    glm::vec3 direction;
    glm::vec3 velocity;  // NUEVO: velocidad actual del proyectil
    float speed;
    float radius;
};
