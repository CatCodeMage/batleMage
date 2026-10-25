#pragma once

#include "Collision.h"
#include "Damage.h"
#include "asset.h"
#include "mapa.h"
#include "../../posibleProyecto/personaje.h"

class CombatManager
{
private:
    CollisionManager m_collisionManager;

public:

    //Crear colliders
    SphereCollider crearColliderHechizo(
        const Hechizo& hechizo
    );

    CylinderCollider crearColliderPersonaje(
        const Personaje& personaje
    );

    BoxCollider CombatManager::crearColliderAsset(
        const Assets& asset
    );

    Damage crearDamage(
        const Hechizo& hechizo
    );

    bool procesarImpacto(
        const Hechizo& hechizo,
        Personaje& personaje
    );

    bool procesarImpacto(
        const Hechizo& hechizo,
        Assets& asset
    );

};