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
void ControladorMaquinaEstados::setcatalogoHechizos(const std::vector<InfoHechizo>& catalogo)
{
    m_catalogoHechizos = catalogo;
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
            if (m_tiempoDesdeUltAtaque >= m_cooldownAtaque)
            {
                personaje.borrarEstado(EstadoPersonaje::IDLE);

                //ELEGIMOS UNO RANDOM DE LOS LANZABLES DE MOMENTO
                int indiceRandom = std::rand() % m_hechizosLanzables.size();
                InfoHechizo hechizoElegido = m_hechizosLanzables[indiceRandom];
                personaje.disminuirMana(hechizoElegido.costeMana);
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

        m_catalogoHechizos.push_back(info);
    }
}

void ControladorMaquinaEstados::apuntarHacia(Personaje& personaje, const glm::vec3& direccion)
{
    personaje.setRotacion(direccion);
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