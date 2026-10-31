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

// Apunta adonde estara el rival cuando llegue el hechizo, suponiendo que sigue moviendose igual
glm::vec3 ControladorMaquinaEstados::calcularDireccionAnticipada(const Personaje& personaje, float velocidadHechizo) const
{
    const glm::vec3 posicionPropia = personaje.getPosicion();
    const glm::vec3 posicionObjetivo = m_objetivo->getPosicion();
    const float segundosDeVuelo = glm::length(posicionObjetivo - posicionPropia) / velocidadHechizo;
    const glm::vec3 posicionPrevista = posicionObjetivo + m_velocidadObjetivo * segundosDeVuelo;
    return glm::normalize(posicionPrevista - posicionPropia);
}

void ControladorMaquinaEstados::actualizar(Personaje& personaje, float deltaTime)
{
    if(personaje.yaTieneEstado(EstadoPersonaje::MUERTO) || personaje.yaTieneEstado(EstadoPersonaje::CONGELADO))
    {
        return;
    }
    m_tiempoDesdeUltAtaque += deltaTime;
    personaje.aumentarMana(m_regenMana*deltaTime);
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

void ControladorMaquinaEstados::evaluarTransiciones(const Personaje& personaje, float distanciaAlObjetivo) 
{
    m_hechizosLanzables.clear();
    std::vector<InfoHechizo> hechizosConMana;
    float manaAct = personaje.getMana();

    for(const InfoHechizo& hechizo : m_catalogoHechizos)
    {
        if(manaAct >= hechizo.costeMana)
        {
            hechizosConMana.push_back(hechizo);
        }
    }

    if (hechizosConMana.empty() || distanciaAlObjetivo < m_distanciaSegura)
    {
        m_estadoIA = EstadoIA::HUYENDO;
        return;
    }

    for(const InfoHechizo& hechizo : hechizosConMana)
    {
        if(distanciaAlObjetivo <= hechizo.alcance)
        {
            m_hechizosLanzables.push_back(hechizo);
        }
    }

    if(m_hechizosLanzables.empty())
    {
        m_estadoIA = EstadoIA::PERSIGUIENDO;
    }
    else
    {
        m_estadoIA = EstadoIA::ATACANDO;
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
            personaje.borrarEstado(EstadoPersonaje::VOLANDO);
            apuntarHacia(personaje, direccionHaciaObjetivo);

            constexpr float DISTANCIA_COMBATE_PREFERIDA = 12.0f;   // mayor que el safeDistance maximo (9): no se pone a huir
            if (distancia > DISTANCIA_COMBATE_PREFERIDA)
                acercarseAlObjetivo(personaje, direccionHaciaObjetivo, deltaTime);   // se acerca mientras lanza: entran mas hechizos en alcance

            rodearObjetivo(personaje, direccionHaciaObjetivo, deltaTime);

            if (m_tiempoDesdeUltAtaque >= m_cooldownAtaque)
            {
                personaje.borrarEstado(EstadoPersonaje::IDLE);

                //ELEGIMOS UNO RANDOM DE LOS LANZABLES DE MOMENTO
                int indiceRandom = std::rand() % m_hechizosLanzables.size();
                InfoHechizo hechizoElegido = m_hechizosLanzables[indiceRandom];
                personaje.disminuirMana(hechizoElegido.costeMana);
                apuntarHacia(personaje, calcularDireccionAnticipada(personaje, hechizoElegido.velocidad));
                personaje.lanzarHechizo(hechizoElegido.id);

                m_cooldownAtaque = hechizoElegido.cooldown;
                m_tiempoDesdeUltAtaque = 0.0f;
            } else {
                if (m_tiempoDesdeUltAtaque > 0.3f) 
                {
                    personaje.borrarEstado(EstadoPersonaje::CASTEANDO);
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