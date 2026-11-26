#include "CombatManager.h"


Damage CombatManager::crearDamage(
    const SpellDefinitionComponent& hechizo)
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

    case ElementType::Lightning:
        return Damage(
            hechizo.damage,
            ElementType::Lightning
        );

    case ElementType::Wind:
        return Damage(
            hechizo.damage,
            ElementType::Wind
        );
    case ElementType::Water:
        return Damage(
            hechizo.damage,
            ElementType::Water
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

    Damage damage =
        crearDamage(hechizo);

    personaje.recibirDano(
        damage.getAmount()
    );

    return true;
}


BoxCollider CombatManager::crearColliderAsset(
    const Assets& asset)
{
    BoxCollider collider;

    collider.center =
        asset.getPosicion();

    glm::vec3 escalaAsset =
        asset.getEscala();

    collider.halfSize =
        escalaAsset / 2.0f;

    return collider;
}


CollisionInfo CombatManager::procesarImpacto(
    const SphereCollider& hechizoCollider,
    Assets& asset)
{
    BoxCollider assetCollider =
        crearColliderAsset(asset);

    CollisionInfo info =
        m_collisionManager.getCollisionInfo(
            assetCollider,
            hechizoCollider
        );

    if (!info.collision)
    {
        return info;
    }

    // Si el objeto puede destruirse,
    // cambiamos su estado
    if (asset.esDestruible())
    {
        switch (asset.getTipo())
        {
        case tipoAsset::ARBOL:

            asset.setEstado(
                estadoArbol::DESTRUIDO
            );

            break;
        }
    }

    return info;
}