#include "CombatManager.h"

Damage CombatManager::crearDamage(const SpellDefinitionComponent& hechizo)
{
    switch (hechizo.element)
    {
    case ElementType::Physical:
        return Damage(
            hechizo.damage * 1.25f,
            ElementType::Physical
        );

    case ElementType::Fire:
        return Damage(
            hechizo.damage,
            ElementType::Fire,
            EffectType::Burn,
            5.0f,
            4.0f
        );

    case ElementType::ICE:
        return Damage(
            hechizo.damage,
            ElementType::ICE,
            EffectType::Slow,
            0.30f,
            3.0f
        );
    }

    return Damage(
        hechizo.damage,
        ElementType::Physical
    );
}

bool CombatManager::procesarImpacto(
    const SphereCollider& hechizoCollider,
    const CylinderCollider& personajeCollider,
    const SpellDefinitionComponent& hechizo,
    Personaje& personaje)
{
    if (!m_collisionManager.checkCollision(
        hechizoCollider,
        personajeCollider))
    {
        return false;
    }

    Damage damage = crearDamage(hechizo);

    personaje.recibirDano(
        damage.getAmount()
    );

    return true;
}