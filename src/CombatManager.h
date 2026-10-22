#pragma once

#include "Collision.h"
#include "Damage.h"
#include "hechizos.h"
#include "personaje.h"

class CombatManager
{
private:
    CollisionManager m_collisionManager;

public:

    SphereCollider crearColliderHechizo(
        const Hechizo& hechizo
    );

    Damage crearDamage(
        const Hechizo& hechizo
    );

    bool procesarImpacto(
        const Hechizo& hechizo,
        Personaje& personaje
    );

    CylinderCollider crearColliderPersonaje(
        const Personaje& personaje
    );

};