#include "varita.h"
#include "lanzadorHechizos.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

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

bool Varita::intentarDisparar(const glm::vec3& posicionMago, float radioMago, float margenCasteo,
                              const glm::vec3& direccionApuntado, HechizosGestor* gestorHechizos, int idLanzador)
{
    if (gestorHechizos == nullptr || m_tiempoEspera > 0.0f || m_ranuras.empty()) {
        return false;
    }

    glm::vec3 direccionFinal = aplicarDispersion(direccionApuntado);
    float distanciaSeparacion = radioMago + margenCasteo;
    glm::vec3 origen = posicionMago + direccionFinal * distanciaSeparacion;

    LanzadorHechizos lanzador(*gestorHechizos);
    ResultadoDisparo r = lanzador.disparar(m_ranuras, m_indiceActual, m_manaActual, origen, direccionFinal, idLanzador);

    m_manaActual = std::max(0.0f, m_manaActual - r.manaGastado);
    m_indiceActual = r.nuevoIndice;

    m_tiempoEspera = std::max(0.0f, m_castDelay + r.castDelay);
    if (r.hayQueRecargar) {
        m_tiempoEspera = std::max(m_tiempoEspera, m_rechargeTime + r.recarga);
    }

    return r.disparado;
}

bool Varita::puedeAtacarObjetivo(float distanciaAlObjetivo, const HechizosGestor& gestor) const
{
    if (m_ranuras.empty()) return false;

    const auto& catalogo = gestor.getCatalogoHechizos();
    const int total = static_cast<int>(m_ranuras.size());

    for (int i = 0; i < total; ++i) {
        int idx = (m_indiceActual + i) % total;
        int idHechizo = m_ranuras[idx];
        if (idHechizo < 0) continue;

        auto it = catalogo.find(idHechizo);
        if (it != catalogo.end()) {
            const hechizosTemplate& info = it->second;
            float alcance = info.speed * info.defaultLifeTime;
            if (alcance <= 0.001f) {
                alcance = info.radius;
            }
            if (distanciaAlObjetivo <= alcance) {
                return true;
            }
        }
    }
    return false;
}

float Varita::getVelocidadProximoHechizo(const HechizosGestor& gestor) const
{
    if (m_ranuras.empty()) return 0.0f;

    const auto& catalogo = gestor.getCatalogoHechizos();
    const int total = static_cast<int>(m_ranuras.size());

    for (int i = 0; i < total; ++i) {
        int idx = (m_indiceActual + i) % total;
        int idHechizo = m_ranuras[idx];
        if (idHechizo < 0) continue;

        auto it = catalogo.find(idHechizo);
        if (it != catalogo.end()) {
            return it->second.speed;
        }
    }
    return 0.0f;
}
