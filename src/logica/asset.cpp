#include "asset.h"

Assets::Assets(tipoAsset tipo, estadoAsset estado, glm::vec3 coordenadas)
    : m_tipo(tipo), m_estado(estado), m_coordenadas(coordenadas)
{
}

estadoAsset Assets::getEstado() const
{
    return m_estado;
}

tipoAsset Assets::getTipo() const
{
    return m_tipo;
}

glm::vec3 Assets::getPosicion() const
{
    return m_coordenadas;
}

glm::vec3 Assets::getRotacion() const
{
    return m_rotacion;
}

glm::vec3 Assets::getEscala() const
{
    return m_escala;
}

bool Assets::esDestruible() const
{
    return m_destruible;
}

void Assets::setEstado(estadoAsset estado)
{
    m_estado = estado;
}

void Assets::setCoordenadas(glm::vec3 coordenadas)
{
    m_coordenadas = coordenadas;
}

void Assets::setRotacion(glm::vec3 rotacion)
{
    m_rotacion = rotacion;
}

void Assets::setEscala(glm::vec3 escala)
{
    m_escala = escala;
}

void Assets::setDestruible(bool destruible)
{
    m_destruible = destruible;
}

void Assets::trasladar(glm::vec3 desplazamiento)
{
    m_coordenadas += desplazamiento;
}

void Assets::incX(float incremento)
{
    m_coordenadas.x += incremento;
}

void Assets::incY(float incremento)
{
    m_coordenadas.y += incremento;
}

void Assets::incZ(float incremento)
{
    m_coordenadas.z += incremento;
}

bool Assets::tieneFuenteDeLuz() const
{
    return m_tieneFuenteDeLuz;
}

const FuenteDeLuz* Assets::getFuenteDeLuz() const
{
    return m_tieneFuenteDeLuz ? &m_fuenteDeLuz : nullptr;
}

void Assets::setFuenteDeLuz(const FuenteDeLuz& fuente)
{
    m_fuenteDeLuz = fuente;
    m_tieneFuenteDeLuz = true;
}

void Assets::quitarFuenteDeLuz()
{
    m_tieneFuenteDeLuz = false;
}
