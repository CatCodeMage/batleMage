#include "mapa.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

Map::Map(tipoMapa tipo, glm::vec3 dimensiones, skybox cielo,
         llumAmbient luz, float alturaSuelo)
    : m_tipo(tipo),
      m_dimensiones(dimensiones),
      m_alturaSuelo(alturaSuelo),
      m_skybox(cielo),
      m_llumAmbient(luz),
      m_skyboxInicial(cielo),
      m_llumAmbientInicial(luz),
      m_alturaSueloInicial(alturaSuelo)
{
    if (!std::isfinite(dimensiones.x) || !std::isfinite(dimensiones.y) ||
        !std::isfinite(dimensiones.z) || !std::isfinite(alturaSuelo) ||
        dimensiones.x <= 0.0f || dimensiones.y < 0.0f || dimensiones.z <= 0.0f)
    {
        throw std::invalid_argument("Las dimensiones del mapa o la altura del suelo no son validas");
    }
}

tipoMapa Map::getTipo() const
{
    return m_tipo;
}

glm::vec3 Map::getDimensiones() const
{
    return m_dimensiones;
}

const std::vector<marca>& Map::getMarcas() const
{
    return m_marcas;
}

skybox Map::getSkyBox() const
{
    return m_skybox;
}

llumAmbient Map::getLuzAmbiental() const
{
    return m_llumAmbient;
}

float Map::getAlturaSuelo() const
{
    return m_alturaSuelo;
}

void Map::setColorLlumAmbient(glm::vec3 rgb)
{
    m_llumAmbient.color = rgb;
}

void Map::setIntensitatLlumAmbient(float intensidad)
{
    m_llumAmbient.intensitat = intensidad;
}

void Map::setInsensitatLlumAmbient(float intensidad)
{
    setIntensitatLlumAmbient(intensidad);
}

void Map::setSkyBox(skybox cielo)
{
    m_skybox = cielo;
}

void Map::setAlturaSuelo(float alturaSuelo)
{
    if (!std::isfinite(alturaSuelo))
    {
        throw std::invalid_argument("La altura del suelo debe ser finita");
    }
    m_alturaSuelo = alturaSuelo;
}

void Map::addMarca(const marca& nuevaMarca)
{
    m_marcas.push_back(nuevaMarca);
}

void Map::clearMarcas()
{
    m_marcas.clear();
}

bool Map::estaDentroDelMapa(glm::vec3 posicion) const
{
    if (!std::isfinite(posicion.x) || !std::isfinite(posicion.z))
    {
        return false;
    }

    const float mitadAncho = m_dimensiones.x / 2.0f;
    const float mitadLargo = m_dimensiones.z / 2.0f;
    return posicion.x >= -mitadAncho && posicion.x <= mitadAncho &&
           posicion.z >= -mitadLargo && posicion.z <= mitadLargo;
}

bool Map::esPosicionValida(glm::vec3 posicion) const
{
    return estaDentroDelMapa(posicion) && std::isfinite(posicion.y) &&
           posicion.y >= m_alturaSuelo &&
           posicion.y <= m_alturaSuelo + m_dimensiones.y;
}

glm::vec3 Map::limitarPosicion(glm::vec3 posicion) const
{
    if (!std::isfinite(posicion.x) || !std::isfinite(posicion.y) ||
        !std::isfinite(posicion.z))
    {
        throw std::invalid_argument("La posicion debe tener coordenadas finitas");
    }

    const float mitadAncho = m_dimensiones.x / 2.0f;
    const float mitadLargo = m_dimensiones.z / 2.0f;

    posicion.x = std::clamp(posicion.x, -mitadAncho, mitadAncho);
    posicion.y = std::clamp(posicion.y, m_alturaSuelo,
                            m_alturaSuelo + m_dimensiones.y);
    posicion.z = std::clamp(posicion.z, -mitadLargo, mitadLargo);
    return posicion;
}

void Map::reiniciarMapa()
{
    clearMarcas();
    m_skybox = m_skyboxInicial;
    m_llumAmbient = m_llumAmbientInicial;
    m_alturaSuelo = m_alturaSueloInicial;
}
