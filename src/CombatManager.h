#pragma once

#include "Collision.h"
#include "Damage.h"
#include "hechizoDatos.h"
#include "personaje.h"
#include "asset.h"

class CombatManager
{
private:
    CollisionManager m_collisionManager;

public:
    Damage crearDamage(
        const SpellDefinitionComponent& hechizo
    );

    // Hechizo contra personaje
    bool procesarImpacto(
        const SphereCollider& hechizoCollider,
        const CylinderCollider& personajeCollider,
        const SpellDefinitionComponent& hechizo,
        Personaje& personaje
    );

    // Crear collider de un asset
    BoxCollider crearColliderAsset(
        const Assets& asset
    );

    // Hechizo contra asset.
    // Devuelve:
    // - si hay colision
    // - normal
    // - penetracion
    CollisionInfo procesarImpacto(
        const SphereCollider& hechizoCollider,
        Assets& asset
    );
};