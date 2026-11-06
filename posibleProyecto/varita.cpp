#include "varita.h"
#include <cmath>
#include <cstdlib>

Varita::Varita(const std::string& nombre, float castDelay, float rechargeTime,
               float manaMax, float recargaMana, int capacidad, float dispersion)
    : m_nombre(nombre),
      m_castDelay(castDelay),
      m_rechargeTime(rechargeTime),
      m_manaMax(manaMax),
      m_manaActual(manaMax),
      m_recargaMana(recargaMana),
      m_capacidad(capacidad > 0 ? capacidad : 1),
      m_dispersion(dispersion),
      m_ranuras(m_capacidad, -1),
      m_indiceActual(0),
      m_tiempoEspera(0.0f)
{
}

void Varita::update(float deltaTime)
{
    m_manaActual += m_recargaMana * deltaTime;
    if (m_manaActual > m_manaMax) {
        m_manaActual = m_manaMax;
    }

    if (m_tiempoEspera > 0.0f) {
        m_tiempoEspera -= deltaTime;
        if (m_tiempoEspera < 0.0f) {
            m_tiempoEspera = 0.0f;
        }
    }
}

bool Varita::equiparHechizoEnRanura(int indiceRanura, int idHechizo)
{
    if (indiceRanura < 0 || indiceRanura >= m_capacidad) return false;
    m_ranuras[indiceRanura] = idHechizo;
    return true;
}

void Varita::limpiarRanura(int indiceRanura)
{
    if (indiceRanura >= 0 && indiceRanura < m_capacidad) {
        m_ranuras[indiceRanura] = -1;
    }
}

//REVISAR!!!!!!!!!!!!!!!!!!
glm::vec3 Varita::aplicarDispersion(const glm::vec3& direccionBase) const
{
    if (m_dispersion <= 0.001f) return glm::normalize(direccionBase);

    // Generamos un ángulo aleatorio entre [-m_dispersion, +m_dispersion] en radianes
    float randNorm = (static_cast<float>(std::rand()) / RAND_MAX) * 2.0f - 1.0f;
    float anguloRad = randNorm * (m_dispersion * 3.14159265f / 180.0f);

    float cosA = std::cos(anguloRad);
    float sinA = std::sin(anguloRad);

    // Rotamos el vector de disparo en el plano horizontal XZ
    glm::vec3 dirRotada(
        direccionBase.x * cosA - direccionBase.z * sinA,
        direccionBase.y,
        direccionBase.x * sinA + direccionBase.z * cosA
    );

    return glm::normalize(dirRotada);
}
