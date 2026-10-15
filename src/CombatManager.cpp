#include "CombatManager.h"

Damage CombatManager::crearDamage(const SpellData& hechizo)
{
    switch (hechizo.element)
    {
    case ElementType::Physical:
        return Damage(
            hechizo.baseDamage * 1.25f,
            ElementType::Physical
        );

    case ElementType::Fire:
        return Damage(
            hechizo.baseDamage,
            ElementType::Fire,
            EffectType::Burn,
            5.0f,
            4.0f
        );

    case ElementType::ICE:
        return Damage(
            hechizo.baseDamage,
            ElementType::ICE,
            EffectType::Slow,
            0.30f,
            3.0f
        );
    }

    return Damage(
        hechizo.baseDamage,
        ElementType::Physical
    );
}

bool CombatManager::procesarImpacto(
    const SphereCollider& hechizoCollider,
    const CylinderCollider& personajeCollider,
    const SpellData& hechizo,
    Personaje& personaje)
{
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

    return true;
}