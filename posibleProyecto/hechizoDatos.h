#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <memory>

// estructura 3D
struct Vector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // Constructores
    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    // --- OPERACIONES MATEMÁTICAS CLAVE ---

    // Suma de vectores (ej: Posición + Velocidad)
    Vector3 operator+(const Vector3& o) const { 
        return Vector3(x + o.x, y + o.y, z + o.z); 
    }

    // Resta de vectores (ej: Objetivo - Origen para hallar la dirección)
    Vector3 operator-(const Vector3& o) const { 
        return Vector3(x - o.x, y - o.y, z - o.z); 
    }

    // Multiplicación por escalar (ej: Dirección * Velocidad * deltaTime)
    Vector3 operator*(float scalar) const { 
        return Vector3(x * scalar, y * scalar, z * scalar); 
    }

    // Módulo / Magnitud (Longitud real del vector en el mundo)
    float Length() const { 
        return std::sqrt(x * x + y * y + z * z); 
    }

    // Normalización: convierte el vector en un "Vector Unitario" (longitud 1)
    // Útil para representar solo la DIRECCIÓN sin importar la distancia.
    Vector3 Normalized() const {
        float len = Length();
        return (len > 0.0001f) ? Vector3(x / len, y / len, z / len) : Vector3(0, 0, 0);
    }
};


enum class ElementType { Physical, Fire, Water, Earth, Air, Lightning };

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

        case ElementType::Air:
            // El aire es muy rápido y reduce el cooldown
            return { 0.9f, 1.5f, 0.5f, 0.1f, "knockback" };

        case ElementType::Water:
            // El agua aplica estado empapado para combos
            return { 1.0f, 1.1f, 0.0f, 1.0f, "wet" };

        case ElementType::Earth:
            // La tierra pega duro pero es lenta
            return { 1.4f, 0.7f, -0.2f, 0.2f, "stunt" };

        case ElementType::Lightning:
            // El rayo es instantáneo / muy rápido con alto crítico
            return { 1.1f, 2.0f, 0.2f, 0.3f, "shocked" };

        case ElementType::Physical:
        default:
            return { 1.0f, 1.0f, 0.0f, 0.0f, "none" };
    }
}

//bonificación contra elementos o estados
inline float GetElementalMultiplier(ElementType attacker, ElementType defender) {
    // [Atacante][Defensor]
    if (attacker == ElementType::Water && defender == ElementType::Fire) return 2.0f; // Agua apaga Fuego
    if (attacker == ElementType::Fire && defender == ElementType::Earth) return 1.5f; // Fuego quema Tierra
    if (attacker == ElementType::Lightning && defender == ElementType::Water) return 2.0f; // Rayo electrocuta Agua
    if (attacker == ElementType::Fire && defender == ElementType::Water) return 0.5f; // Fuego contra agua hace menos
    
    return 1.0f; // Daño normal por defecto
}

enum class CastType { Projectile, AreaOfEffect, Cone, Raycast, Self };
enum class MovementPattern { Linear, Homing, Parabolic, Orbiting, Static };

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


// ============================================================================
// CONTEXTO DE COLISIÓN (Soluciona el error 'identifier CollisionContext is undefined') //solo para usado para probar
// ============================================================================
// solo para mi imaginarme como podria ser, creado por chat 
struct CollisionContext {
    Vector3 hitPoint;
    Vector3 hitNormal;
    ElementType objectElement = ElementType::Physical;
    std::string objectState = "none"; // Ej: "burned", "wet", "frozen"
    void* hitCollider = nullptr;      // Puntero genérico al objeto impactado
};

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
    if (attackElement == ElementType::Water) {
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
    else if (attackElement == ElementType::Earth) {
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