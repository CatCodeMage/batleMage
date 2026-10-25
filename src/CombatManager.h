#pragma once

#include "Collision.h"
#include "Damage.h"
#include "hechizoDatos.h"
#include "personaje.h"

class CombatManager
{
private:
    CollisionManager m_collisionManager;

public:
    Damage crearDamage(const SpellDefinitionComponent& hechizo);

    bool procesarImpacto(
        const SphereCollider& hechizoCollider,
        const CylinderCollider& personajeCollider,
        const SpellDefinitionComponent& hechizo,
        Personaje& personaje
    );
};