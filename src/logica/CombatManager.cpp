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

    SphereCollider personajeCollider =
        crearColliderPersonaje(personaje);

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

SphereCollider CombatManager::crearColliderPersonaje(
    const Personaje& personaje)
{
    SphereCollider collider;

    collider.center = personaje.getPosicion();
    collider.radius = personaje.getRadio();

    return collider;
}

BoxCollider CombatManager::crearColliderAsset(
    const Assets& asset)
{
    BoxCollider collider;

    collider.center = asset.getPosicion();

    glm::vec3 escalaAsset = asset.getEscala();
    escalaAsset /= 2;

    collider.halfSize = escalaAsset;

    return collider;
}

bool CombatManager::procesarImpacto(
    const Hechizo& hechizo,
    Assets& asset)
{
    SphereCollider hechizoCollider =
        crearColliderHechizo(hechizo);

    BoxCollider assetCollider =
        crearColliderAsset(asset);

    if (!m_collisionManager.checkCollision(
        assetCollider,
        hechizoCollider))
    {
        return false;
    }

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

    return true;
}

bool CombatManager::procesarImpacto(
    const Hechizo& hechizo,
    Map& mapa)
{
    glm::vec3 posicion = hechizo.getPosition();
    float radio = hechizo.getRadius();

    // El hechizo debe encontrarse dentro del mapa
    if (!mapa.estaDentroDelMapa(posicion))
    {
        return false;
    }

    // Comprobamos si la parte inferior de la esfera toca el suelo
    if (posicion.y - radio > mapa.getAlturaSuelo())
    {
        return false;
    }

    // Si impacta, generamos una marca según el elemento
    if (hechizo.getElement() == ElementType::Fire)
    {
        marca nuevaMarca;

        nuevaMarca.coordenadas = posicion;
        nuevaMarca.radio = radio;
        nuevaMarca.tipo = tipoMarca::FUEGO;

        mapa.addMarca(nuevaMarca);
    }
    else if (hechizo.getElement() == ElementType::ICE)
    {
        marca nuevaMarca;

        nuevaMarca.coordenadas = posicion;
        nuevaMarca.radio = radio;
        nuevaMarca.tipo = tipoMarca::HIELO;

        mapa.addMarca(nuevaMarca);
    }

    return true;
}

