#include "controladorMaquinaEstados.h"
#include <cmath>
#include <iostream>

ControladorMaquinaEstados::ControladorMaquinaEstados(Personaje* objetivo, float distanciaAtaque, float distanciaSegura,
    float velocidadMovimiento, float costeManaHechizo, float cooldownAtaque, float regeneracionManaSeg) : 
                m_objetivo(objetivo),
                m_estadoIA(EstadoIA::ESPERANDO),
                m_distanciaAtaque(distanciaAtaque),
                m_distanciaSegura(distanciaSegura),
                m_velocidadMovimiento(velocidadMovimiento),
                m_costeManaHechizo(costeManaHechizo),
                m_cooldownAtaque(cooldownAtaque),
                m_tiempoDesdeUltAtaque(cooldownAtaque),
                m_regenMana(regeneracionManaSeg)
{
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
    Vector3 posPropia = personaje.getPosicion();
    Vector3 posObj = m_objetivo->getPosicion();
    Vector3 diff(posObj.x - posPropia.x, 
                posObj.y - posPropia.y, 
                posObj.z - posPropia.z);
    float distancia = std::sqrt(diff.x * diff.x + diff.y * diff.y + diff.z * diff.z);
    Vector3 direccion(0.0f, 0.0f, 0.0f);
    if(distancia > 0.0001f)
    {
        direccion = Vector3(diff.x / distancia, diff.y / distancia, diff.z / distancia);
    }
    evaluarTransiciones(personaje, distancia);
    ejecutarEstado(personaje, direccion, distancia, deltaTime);
}

void ControladorMaquinaEstados::evaluarTransiciones(const Personaje& personaje, float distanciaAlObjetivo) 
{
    bool tieneManaParaAtacar = (personaje.getMana() >= m_costeManaHechizo);
    // Si no tiene maná o el enemigo está demasiado cerca -> HUIR para recargar/reposicionarse
    if (!tieneManaParaAtacar || distanciaAlObjetivo < m_distanciaSegura) 
    {
        m_estadoIA = EstadoIA::HUYENDO;
    }
    // Si el enemigo está más lejos que nuestro alcance de hechizo -> PERSEGUIR
    else if (distanciaAlObjetivo > m_distanciaAtaque) 
    {
        m_estadoIA = EstadoIA::PERSIGUIENDO;
    }
    // Si está en rango óptimo y tiene maná -> ATACAR
    else 
    {
        m_estadoIA = EstadoIA::ATACANDO;
    }
}

void ControladorMaquinaEstados::ejecutarEstado(Personaje& personaje,const Vector3& direccionHaciaObjetivo,
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

            Vector3 desplazamiento(
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

            Vector3 desplazamientoHuida(
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
                personaje.disminuirMana(m_costeManaHechizo);
                personaje.lanzarHechizo();
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

void ControladorMaquinaEstados::apuntarHacia(Personaje& personaje, const Vector3& direccion)
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