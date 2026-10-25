#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <memory>
#include "global.h" 
#include "CombatTypes.h"

enum class CastType { Projectile, AreaOfEffect, Cone, Raycast, Self };
enum class MovementPattern { Linear, Homing, Parabolic, Orbiting, Static };

// 2. El componente aloja el enum junto al resto de datos estáticos
// he sacado del anterior struct los que cambia
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

// Componentes de estado dinámico
struct ProjectileComponent {
    glm::vec3 translacio;
    glm::vec3 rotacio;
    glm::vec3 scale;
    glm::vec3 direction;
    float speed;
    float radius;
};

//-----------------------------------------***






