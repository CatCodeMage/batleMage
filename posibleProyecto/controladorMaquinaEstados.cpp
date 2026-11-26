#include "controladorMaquinaEstados.h"
#include <cmath>
#include <iostream>

ControladorMaquinaEstados::ControladorMaquinaEstados(Personaje* objetivo,
    const std::vector<InfoHechizo>& catalogoHechizos, float distanciaSegura,
    float velocidadMovimiento, float cooldownAtaque, float regeneracionManaSeg)
{
    m_objetivo = objetivo;
    m_catalogoHechizos = catalogoHechizos; 
    m_estadoIA = EstadoIA::ESPERANDO;
    m_distanciaSegura = distanciaSegura;
    m_velocidadMovimiento = velocidadMovimiento;
    m_cooldownAtaque = cooldownAtaque;
    m_regenMana = regeneracionManaSeg;
    m_tiempoDesdeUltAtaque = cooldownAtaque;
}

// Se mueve de lado alrededor del rival; cambia de sentido cada pocos segundos para no ser predecible
void ControladorMaquinaEstados::rodearObjetivo(Personaje& personaje, const glm::vec3& direccionHaciaObjetivo, float deltaTime)
{
    constexpr float FRACCION_VELOCIDAD_RODEO = 0.7f;
    constexpr float SEGUNDOS_MINIMOS_POR_SENTIDO = 1.5f;
    constexpr float SEGUNDOS_EXTRA_AL_AZAR = 2.0f;

    m_tiempoHastaCambiarSentido -= deltaTime;
    if (m_tiempoHastaCambiarSentido <= 0.0f) {
        m_sentidoRodeo = (std::rand() % 2 == 0) ? 1.0f : -1.0f;
        const float azarEntre0y1 = static_cast<float>(std::rand()) / RAND_MAX;
        m_tiempoHastaCambiarSentido = SEGUNDOS_MINIMOS_POR_SENTIDO + azarEntre0y1 * SEGUNDOS_EXTRA_AL_AZAR;
    }

    const glm::vec3 direccionLateral(direccionHaciaObjetivo.z, 0.0f, -direccionHaciaObjetivo.x);   // perpendicular en el suelo
    personaje.moverse(direccionLateral * m_sentidoRodeo * m_velocidadMovimiento * FRACCION_VELOCIDAD_RODEO * deltaTime);
}

void ControladorMaquinaEstados::setcatalogoHechizos(const std::vector<InfoHechizo>& catalogo)
{
    m_catalogoHechizos = catalogo;
}

glm::vec3 ControladorMaquinaEstados::calcularDireccionAnticipada(const Personaje& personaje, float velocidadHechizo) const
{
    const glm::vec3 posicionPropia = personaje.getPosicion();
    const glm::vec3 posicionObjetivo = m_objetivo->getPosicion();
    const glm::vec3 diff = posicionObjetivo - posicionPropia;
    if (glm::length(diff) <= 0.0001f) {
        return glm::vec3(0.0f, 0.0f, 1.0f);
    }
    if (velocidadHechizo <= 0.001f) {
        return glm::normalize(diff);
    }
    const float segundosDeVuelo = glm::length(diff) / velocidadHechizo;
    const glm::vec3 posicionPrevista = posicionObjetivo + m_velocidadObjetivo * segundosDeVuelo;
    const glm::vec3 dir = posicionPrevista - posicionPropia;
    return (glm::length(dir) > 0.0001f) ? glm::normalize(dir) : glm::normalize(diff);
}

void ControladorMaquinaEstados::actualizar(Personaje& personaje, float deltaTime)
{
    if(personaje.yaTieneEstado(EstadoPersonaje::MUERTO) || personaje.yaTieneEstado(EstadoPersonaje::CONGELADO))
    {
        return;
    }
    m_tiempoDesdeUltAtaque += deltaTime;
    if(m_objetivo == nullptr || m_objetivo->yaTieneEstado(EstadoPersonaje::MUERTO))
    {
        m_estadoIA = EstadoIA::ESPERANDO;
        personaje.borrarEstado(EstadoPersonaje::VOLANDO);
        personaje.borrarEstado(EstadoPersonaje::CASTEANDO);
        personaje.anadirEstado(EstadoPersonaje::IDLE);
        return;
    }
    glm::vec3 posPropia = personaje.getPosicion();
    glm::vec3 posObj = m_objetivo->getPosicion();

    // Velocidad del rival a partir de cuanto se ha movido desde el tick anterior
    if (m_conocePosicionAnteriorObjetivo && deltaTime > 0.0f)
        m_velocidadObjetivo = (posObj - m_posicionAnteriorObjetivo) / deltaTime;
    m_posicionAnteriorObjetivo = posObj;
    m_conocePosicionAnteriorObjetivo = true;

    glm::vec3 diff(posObj.x - posPropia.x, 
                posObj.y - posPropia.y, 
                posObj.z - posPropia.z);
    float distancia = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    glm::vec3 direccion(0.0f, 0.0f, 0.0f);
    if(distancia > 0.0001f)
    {
        direccion = glm::vec3(diff.x / distancia, diff.y / distancia, diff.z / distancia);
    }
    evaluarTransiciones(personaje, distancia);
    ejecutarEstado(personaje, direccion, distancia, deltaTime);
}

void ControladorMaquinaEstados::evaluarTransiciones(Personaje& personaje, float distanciaAlObjetivo)
{
    if (personaje.yaTieneEstado(EstadoPersonaje::CASTEANDO))
    {
        return;
    }

    const HechizosGestor* gestor = personaje.getGestorHechizos();
    const auto& varitas = personaje.getVaritas();

    if (gestor == nullptr || varitas.empty() || distanciaAlObjetivo < m_distanciaSegura)
    {
        m_estadoIA = EstadoIA::HUYENDO;
        return;
    }

    int mejorVaritaLista = -1;
    int mejorVaritaEnRango = -1;
    bool algunaTieneMana = false;

    for (int i = 0; i < static_cast<int>(varitas.size()); ++i)
    {
        // Con distancia 0 comprobamos si la varita tiene maná para lanzar algún hechizo
        if (!varitas[i].puedeAtacarObjetivo(0.0f, *gestor))
        {
            continue;
        }
        algunaTieneMana = true;

        if (varitas[i].puedeAtacarObjetivo(distanciaAlObjetivo, *gestor))
        {
            if (mejorVaritaEnRango < 0)
            {
                mejorVaritaEnRango = i;
            }
            if (varitas[i].estaLista() && mejorVaritaLista < 0)
            {
                mejorVaritaLista = i;
            }
        }
    }

    if (mejorVaritaLista >= 0)
    {
        personaje.cambiarVarita(mejorVaritaLista);
        m_estadoIA = EstadoIA::ATACANDO;
    }
    else if (mejorVaritaEnRango >= 0)
    {
        personaje.cambiarVarita(mejorVaritaEnRango);
        m_estadoIA = EstadoIA::ATACANDO;
    }
    else if (algunaTieneMana)
    {
        m_estadoIA = EstadoIA::PERSIGUIENDO;
    }
    else
    {
        m_estadoIA = EstadoIA::HUYENDO;
    }
}

void ControladorMaquinaEstados::ejecutarEstado(Personaje& personaje,const glm::vec3& direccionHaciaObjetivo,
    float distancia, float deltaTime)
{
    switch (m_estadoIA) 
    {
        case EstadoIA::ESPERANDO:
            personaje.borrarEstado(EstadoPersonaje::VOLANDO);
            personaje.borrarEstado(EstadoPersonaje::CASTEANDO);
            personaje.anadirEstado(EstadoPersonaje::IDLE);
            break;

        case EstadoIA::PERSIGUIENDO: 
        {
            personaje.borrarEstado(EstadoPersonaje::CASTEANDO);
            apuntarHacia(personaje, direccionHaciaObjetivo);

            glm::vec3 desplazamiento(
                direccionHaciaObjetivo.x * m_velocidadMovimiento * deltaTime,
                direccionHaciaObjetivo.y * m_velocidadMovimiento * deltaTime,
                direccionHaciaObjetivo.z * m_velocidadMovimiento * deltaTime
            );
            personaje.moverse(desplazamiento);
            break;
        }

        case EstadoIA::HUYENDO: 
        {
            personaje.borrarEstado(EstadoPersonaje::CASTEANDO);
            apuntarHacia(personaje, direccionHaciaObjetivo); // Sigue mirando al enemigo mientras retrocede

            glm::vec3 desplazamientoHuida(
                -direccionHaciaObjetivo.x * m_velocidadMovimiento * deltaTime,
                -direccionHaciaObjetivo.y * m_velocidadMovimiento * deltaTime,
                -direccionHaciaObjetivo.z * m_velocidadMovimiento * deltaTime
            );
            personaje.moverse(desplazamientoHuida);
            break;
        }

        case EstadoIA::ATACANDO:
        {
            if (personaje.yaTieneEstado(EstadoPersonaje::CASTEANDO))
            {
                break;
            }
            personaje.borrarEstado(EstadoPersonaje::VOLANDO);
            apuntarHacia(personaje, direccionHaciaObjetivo);
            constexpr float DISTANCIA_COMBATE_PREFERIDA = 12.0f;
            if (distancia > DISTANCIA_COMBATE_PREFERIDA)
            {
                acercarseAlObjetivo(personaje, direccionHaciaObjetivo, deltaTime);
            }
            rodearObjetivo(personaje, direccionHaciaObjetivo, deltaTime);
            Varita* varita = personaje.getVaritaActual();
            const HechizosGestor* gestor = personaje.getGestorHechizos();
            if (varita != nullptr && gestor != nullptr && varita->estaLista())
            {
                personaje.borrarEstado(EstadoPersonaje::IDLE);
                const float vel = varita->getVelocidadProximoHechizo(*gestor);
                apuntarHacia(personaje, calcularDireccionAnticipada(personaje, vel));
                if (personaje.dispararVaritaActual())
                {
                    m_tiempoDesdeUltAtaque = 0.0f;
                }
                else
                {
                    personaje.anadirEstado(EstadoPersonaje::IDLE);
                }
            }
            break;
        }
    }
}

void ControladorMaquinaEstados::acercarseAlObjetivo(Personaje& personaje, const glm::vec3& direccionHaciaObjetivo, float deltaTime)
{
    personaje.moverse(direccionHaciaObjetivo * m_velocidadMovimiento * deltaTime);
}

void ControladorMaquinaEstados::cargarCatalogoDesdeGestor(const HechizosGestor& gestor)
{
    m_catalogoHechizos.clear();
    const auto& catalogo = gestor.getCatalogoHechizos();

    for(const auto& par : catalogo)
    {
        int id = par.first;
        const hechizosTemplate& temp = par.second;

        InfoHechizo info;
        info.id = id;
        info.costeMana = temp.definition.cost;
        info.alcance = temp.speed * temp.defaultLifeTime;
        info.cooldown = temp.definition.cooldown;
        info.velocidad = temp.speed;

        m_catalogoHechizos.push_back(info);
    }
}

void ControladorMaquinaEstados::apuntarHacia(Personaje& personaje, const glm::vec3& direccion)
{
    personaje.setDireccion(direccion);

    float anguloY = std::atan2(direccion.x, direccion.z);
    if (anguloY < 0.0f) {
        anguloY += 2.0f * 3.14159265f;
    }
    personaje.setRotacion(glm::vec3(0.0f, anguloY, 0.0f));
}

void ControladorMaquinaEstados::setObjetivo(Personaje* nuevoObjetivo)
{
    m_objetivo = nuevoObjetivo;
}

Personaje* ControladorMaquinaEstados::getObjetivo() const 
{
    return m_objetivo;
}

EstadoIA ControladorMaquinaEstados::getEstadoIA() const 
{
    return m_estadoIA;
}