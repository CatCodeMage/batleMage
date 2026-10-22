#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <memory>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

// estructura 3D
// nueva estructura

// 1. Definimos el enum tipado para los elementos
enum class ElementType {
    None,
    Fire,
    Ice,
    water,
    earth,
    wind,
    Lightning,
    Arcane,
    Poison
};

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

struct ElementBonus {
    float damageMultiplier = 1.0f; // Ej: 1.2f = +20% de daño
    float speedMultiplier  = 1.0f; // Ej: 1.5f = +50% de velocidad de proyectil
    float cooldownReducer  = 0.0f; // Ej: 0.5s menos de cooldown
    float statusChance     = 0.0f; // Probabilidad de aplicar efecto (quemadura, congelar...)
    std::string defaultStatus = "none";
};

//bonificación normal o stats de elementos
inline ElementBonus GetElementBonus(ElementType type){

     switch (type) {
        case ElementType::Fire:
            // El fuego pega más fuerte y puede quemar
            return { 1.25f, 1.0f, 0.0f, 0.4f, "burned" };

        case ElementType::Ice:
            // El aire es muy rápido y reduce el cooldown
            return { 0.9f, 1.5f, 0.5f, 0.1f, "knockback" };

        case ElementType::Poison:
            // El agua aplica estado empapado para combos
            return { 1.0f, 1.1f, 0.0f, 1.0f, "wet" };

        case ElementType::None:
            // La tierra pega duro pero es lenta
            return { 1.4f, 0.7f, -0.2f, 0.2f, "stunt" };

        case ElementType::Lightning:
            // El rayo es instantáneo / muy rápido con alto crítico
            return { 1.1f, 2.0f, 0.2f, 0.3f, "shocked" };

        case ElementType::Arcane:
            return { 1.1f, 2.0f, 0.2f, 0.3f, "shocked" };
        default:
            return { 1.0f, 1.0f, 0.0f, 0.0f, "none" };
    }
}

//bonificación contra elementos o estados
inline float GetElementalMultiplier(ElementType attacker, ElementType defender) {
    // [Atacante][Defensor]
    if (attacker == ElementType::Arcane && defender == ElementType::Fire) return 2.0f; // Agua apaga Fuego
    if (attacker == ElementType::Fire && defender == ElementType::Arcane) return 1.5f; // Fuego quema Tierra
    if (attacker == ElementType::Lightning && defender == ElementType::Arcane) return 2.0f; // Rayo electrocuta Agua
    if (attacker == ElementType::Fire && defender == ElementType::Arcane) return 0.5f; // Fuego contra agua hace menos
    
    return 1.0f; // Daño normal por defecto
}


// este es el viejo, he hecho uno nuevo he quitado algunos, de momento lo dejo para que no me salte rojo las demás cosas de este .h
struct SpellData {
    std::string id = "spell_base";
    std::string name = "Base Spell";

    CastType castType = CastType::Projectile;
    MovementPattern movementPattern = MovementPattern::Linear;
    ElementType element = ElementType::Fire;
    int cooldown = 10;
    int costes = 10;
    // Atributos modificables en el deck/runtime
    float baseDamage = 25.0f;
    float speed = 12.0f;
    float radius = 0.5f;
    float lifetime = 4.0f;
    bool destroyOnImpact = true;
};


enum class estadosElementos{vapor,quemado,mud,electrificado};

enum class combos{magma,tornado,mud,electroBoom};


void ApplyElementBonus(SpellData& data) {
    ElementBonus bonus = GetElementBonus(data.element);

    data.baseDamage *= bonus.damageMultiplier;
    data.speed      *= bonus.speedMultiplier;
    data.cooldown   = std::max(0.1f, data.cooldown - bonus.cooldownReducer);
}


// Reaction elemental a estado que esta
struct ReactionResult {
    estadosElementos nextState;  // El nuevo estado en el que queda el objetivo
    float damageMultiplier = 1.0f; // Modificador de daño (ej: 2.0x para Vapor/ElectroBoom)
    combos comboTriggered = combos::mud; // Identificador del combo para efectos visuales/sonido
    bool hasCombo = false;      // Indica si la reacción produjo una explosión/combo especial
};

inline ReactionResult EvaluateElementalInteraction(ElementType attackElement, estadosElementos currentState) {
    ReactionResult result;

    // --- REACCIONES DE AGUA ---
    if (attackElement == ElementType::Arcane) {
        if (currentState == estadosElementos::quemado) {
            // Agua + Quemado -> Crea Vapor (Extingue el fuego, causa daño extra de vaporización)
            result.nextState = estadosElementos::vapor;
            result.damageMultiplier = 1.5f;
            result.comboTriggered = combos::magma; // O una reacción de Vapor
            result.hasCombo = true;
        } 
        else if (currentState == estadosElementos::electrificado) {
            // Agua + Electrificado -> Conducción masiva
            result.nextState = estadosElementos::electrificado;
            result.damageMultiplier = 2.0f;
            result.comboTriggered = combos::electroBoom;
            result.hasCombo = true;
        }
    }
    // --- REACCIONES DE FUEGO ---
    else if (attackElement == ElementType::Fire) {
        if (currentState == estadosElementos::vapor) {
            // Fuego + Vapor -> Explosión térmica
            result.nextState = estadosElementos::quemado;
            result.damageMultiplier = 1.8f;
            result.comboTriggered = combos::magma;
            result.hasCombo = true;
        }
        else if (currentState == estadosElementos::mud) {
            // Fuego + Barro -> Endurece el barro (Stun o Rompe armadura)
            result.nextState = estadosElementos::quemado;
            result.damageMultiplier = 1.2f;
        }
    }
    // --- REACCIONES DE TIERRA ---
    else if (attackElement == ElementType::Arcane) {
        if (currentState == estadosElementos::vapor || currentState == estadosElementos::mud) {
            // Tierra + Humedad/Vapor -> Barro
            result.nextState = estadosElementos::mud;
            result.damageMultiplier = 1.3f;
            result.comboTriggered = combos::mud;
            result.hasCombo = true;
        }
    }
    // --- REACCIONES DE RAYO ---
    else if (attackElement == ElementType::Lightning) {
        if (currentState == estadosElementos::vapor) {
            // Rayo + Vapor/Humedad -> Explosión eléctrica
            result.nextState = estadosElementos::electrificado;
            result.damageMultiplier = 2.5f;
            result.comboTriggered = combos::electroBoom;
            result.hasCombo = true;
        }
    }

    return result;
}


