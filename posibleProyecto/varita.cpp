#include "varita.h"
#include "lanzadorHechizos.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <variant>

// Helper interno: simula qué cartas leería LanzadorHechizos::leerCartas sin gastar maná ni modificar m_indiceActual
// (usamos getCarta directamente para poder recibir const HechizosGestor&)
static EstadoDisparo simularProximoDisparo(const std::vector<int>& ranuras, int indice,
                                           float manaDisponible, const HechizosGestor& gestor)
{
    EstadoDisparo estado;
    LectorCartas lector;
    const int total = static_cast<int>(ranuras.size());

    while (lector.robosPendientes > 0 && indice < total) {
        const int idCarta = ranuras[indice++];
        if (idCarta < 0) continue;

        const Carta* carta = gestor.getCarta(idCarta);
        if (!carta) continue;
        if (estado.mana + carta->getMana() > manaDisponible) continue;

        estado.mana += carta->getMana();
        estado.castDelay += carta->getCastDelay();
        estado.recarga += carta->getRecarga();
        carta->aplicar(estado, lector);
    }
    return estado;
}

static float calcularAlcanceHechizo(const hechizosTemplate& info, const ModificadoresDisparo& mods)
{
    // Zona directa (aliento, chorro, muro, tornado...)
    if (info.definition.casteo == CastType::AreaOfEffect && info.zona.tieneZona()) {
        if (const Esfera* e = std::get_if<Esfera>(&info.zona.forma)) {
            return e->radio;
        }
        if (const Cono* c = std::get_if<Cono>(&info.zona.forma)) {
            return c->alcance;
        }
        if (const Caja* b = std::get_if<Caja>(&info.zona.forma)) {
            return b->largo;
        }
        if (const Tornado* t = std::get_if<Tornado>(&info.zona.forma)) {
            return (info.zona.velocidadAvance * mods.multVelocidad) * info.zona.duracion + t->radioArriba;
        }
    }

    // Proyectil normal (y proyectil que al explotar deja esfera, como bomb / poison_bomb)
    float alcance = (info.speed * mods.multVelocidad) * (info.defaultLifeTime + mods.lifetimeExtra);
    if (info.zona.tieneZona()) {
        if (const Esfera* e = std::get_if<Esfera>(&info.zona.forma)) {
            alcance += e->radio;
        }
    }
    if (alcance <= 0.001f) {
        alcance = info.radius;
    }
    return alcance;
}

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

void Varita::setRanuras(const std::vector<int>& ranuras)
{
    m_ranuras = ranuras;
    m_capacidad = m_ranuras.empty() ? 1 : static_cast<int>(m_ranuras.size());
    if (m_ranuras.empty()) {
        m_ranuras.assign(1, -1);
    }
    if (m_indiceActual < 0 || m_indiceActual >= m_capacidad) {
        m_indiceActual = 0;
    }
}

void Varita::setManaActual(float mana)
{
    m_manaActual = std::clamp(mana, 0.0f, m_manaMax);
}

void Varita::setIndiceActual(int indice)
{
    m_indiceActual = (indice >= 0 && indice < m_capacidad) ? indice : 0;
}

void Varita::setTiempoEspera(float tiempo)
{
    m_tiempoEspera = std::max(0.0f, tiempo);
}

glm::vec3 Varita::aplicarDispersion(const glm::vec3& direccionBase) const
{
    if (glm::length(direccionBase) <= 0.0001f) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
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

    EstadoDisparo disparo = simularProximoDisparo(m_ranuras, m_indiceActual, m_manaActual, gestor);
    // Si desde m_indiceActual al final solo quedan huecos vacíos (-1), simulamos el ciclo tras recargar
    if (disparo.numProyectiles == 0 && m_indiceActual > 0) {
        disparo = simularProximoDisparo(m_ranuras, 0, m_manaActual, gestor);
    }

    const auto& catalogo = gestor.getCatalogoHechizos();
    for (int i = 0; i < disparo.numProyectiles; ++i) {
        auto it = catalogo.find(disparo.proyectiles[i]);
        if (it == catalogo.end()) continue;

        const hechizosTemplate& info = it->second;
        if (info.definition.casteo == CastType::Self) {
            return true;
        }
        if (distanciaAlObjetivo <= calcularAlcanceHechizo(info, disparo.mods)) {
            return true;
        }
    }
    return false;
}

float Varita::getVelocidadProximoHechizo(const HechizosGestor& gestor) const
{
    if (m_ranuras.empty()) return 0.0f;

    EstadoDisparo disparo = simularProximoDisparo(m_ranuras, m_indiceActual, m_manaActual, gestor);
    if (disparo.numProyectiles == 0 && m_indiceActual > 0) {
        disparo = simularProximoDisparo(m_ranuras, 0, m_manaActual, gestor);
    }

    const auto& catalogo = gestor.getCatalogoHechizos();
    for (int i = 0; i < disparo.numProyectiles; ++i) {
        auto it = catalogo.find(disparo.proyectiles[i]);
        if (it == catalogo.end()) continue;

        const hechizosTemplate& info = it->second;
        if (info.definition.casteo == CastType::Self) {
            continue;
        }
        if (info.definition.casteo == CastType::AreaOfEffect) {
            if (info.zona.velocidadAvance > 0.0f) {
                return info.zona.velocidadAvance * disparo.mods.multVelocidad;
            }
            continue;
        }
        if (info.speed > 0.0f) {
            return info.speed * disparo.mods.multVelocidad;
        }
    }
    return 0.0f;
}