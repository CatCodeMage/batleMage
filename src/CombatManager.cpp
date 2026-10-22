#include "CombatManager.h"

Damage CombatManager::crearDamage(
    const Hechizo& hechizo)
{
    switch (hechizo.getElement())
    {
    case ElementType::Physical:

        return Damage(
            hechizo.getDamage() * 1.25f,
            ElementType::Physical
        );


    case ElementType::Fire:

        return Damage(
            hechizo.getDamage(),
            ElementType::Fire,
            EffectType::Burn,
            5.0f,
            4.0f
        );


    case ElementType::ICE:

        return Damage(
            hechizo.getDamage(),
            ElementType::ICE,
            EffectType::Slow,
            0.30f,
            3.0f
        );
    }

    return Damage(
        hechizo.getDamage(),
        ElementType::Physical
    );
}

bool CombatManager::procesarImpacto(
    const Hechizo& hechizo,
    Personaje& personaje)
{
    SphereCollider hechizoCollider =
        crearColliderHechizo(hechizo);

    CylinderCollider personajeCollider =
        crearColliderPersonaje(personaje);

    if (!m_collisionManager.checkCollision(
        hechizoCollider,
        personajeCollider))
    {
        return false;
    }

    Damage damage = crearDamage(hechizo);

    personaje.recibirDaño(
        damage.getAmount()
    );

    if (damage.getEffect() != EffectType::None)
    {
        personaje.aplicarEfecto(
            damage.getEffect(),
            damage.getEffectValue(),
            damage.getDuration()
        );
    }

    return true;
}

SphereCollider CombatManager::crearColliderHechizo(
    const Hechizo& hechizo)
{
    SphereCollider collider;

    collider.center = hechizo.getPosition();
    collider.radius = hechizo.getRadius();

    return collider;
}

CylinderCollider CombatManager::crearColliderPersonaje(
    const Personaje& personaje)
{
    CylinderCollider collider;

    collider.center = glm::vec3(
        personaje.getX(),
        personaje.getY(),
        personaje.getZ()
    );

    collider.radius = 0.5f;
    collider.halfHeight = 1.0f;

    return collider;
}