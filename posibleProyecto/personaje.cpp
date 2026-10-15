#include "personaje.h"
#include <iostream>
using namespace std;

Personaje::Personaje(int id, Vector3 posicionInicial)
{
    this->m_id = id;
    this->m_posicion = posicionInicial;
    this->m_rotacion = Vector3(0,0,0);
    this->m_vida = 100.f;
    this->m_mana = 50.0f;
    anadirEstado(EstadoPersonaje::IDLE);
    m_controlador = nullptr;
}

void Personaje::anadirEstado(EstadoPersonaje nuevoEstado)
{
    if(!yaTieneEstado(nuevoEstado))
    {
        m_estadosActuales.push_back(nuevoEstado);
    }
}

void Personaje::borrarEstado(EstadoPersonaje estado)
{
    auto it = std::find(m_estadosActuales.begin(), m_estadosActuales.end(), estado);
    if(it != m_estadosActuales.end())
    {
        m_estadosActuales.erase(it);
    }
}

bool Personaje::yaTieneEstado(EstadoPersonaje estado)
{
    return std::find(m_estadosActuales.begin(), m_estadosActuales.end(), estado) != m_estadosActuales.end();
}

void Personaje::update(float deltaTime)
{
    if(yaTieneEstado(EstadoPersonaje::MUERTO))return;
    if(m_controlador != nullptr)
    {
        m_controlador->actualizar(*this, deltaTime);
    }
}

void Personaje::moverse(const Vector3& desplazamiento)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::PARALIZADO)) return;

    m_posicion = m_posicion + desplazamiento;
    
    borrarEstado(EstadoPersonaje::IDLE);
    anadirEstado(EstadoPersonaje::VOLANDO);
}

void Personaje::rotar(const Vector3& rotacion)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::PARALIZADO)) return;

    m_rotacion  = m_rotacion + rotacion;
}

void Personaje::recibirDano(float cantidadDano)
{
    if(yaTieneEstado(EstadoPersonaje::MUERTO)) return;
    m_vida -= cantidadDano;
    cout << "Personaje " << m_id << " recibe " << cantidadDano << " de dano. Vida restante: " << m_vida << endl;

    if (m_vida <= 0)
    {
        m_vida = 0;
        m_estadosActuales.clear();
        anadirEstado(EstadoPersonaje::MUERTO);
        cout << "Personaje " << m_id << " ha muerto." << endl;
    }
}

void Personaje::aumentarVida(float cantidad)
{
    m_vida += cantidad;
}

void Personaje::aumentarMana(float cantidad)
{
    m_mana += cantidad;
}

void Personaje::disminuirMana(float cantidad)
{
    m_mana -= cantidad;
    if(m_mana < 0) m_mana = 0;
}

void Personaje::lanzarHechizo() 
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::PARALIZADO)) return;

    anadirEstado(EstadoPersonaje::CASTEANDO);
    //cout << "Personaje " << m_id << " lanxa un hechizo en la posicion (" << m_x<< "," << m_y << "," << m_z << ")" << endl;
    //pasar projectil a la logica de colisiones
}

void Personaje::setControlador(ControladorPersonaje* nuevoControlador)
 {
    m_controlador = nuevoControlador;
}

int Personaje::getId() const { return m_id; }

float Personaje::getVida() const { return m_vida; }

float Personaje::getMana() const { return m_mana; }

Vector3 Personaje::getPosicion() const { return m_posicion; }
Vector3 Personaje::getRotacion() const { return m_rotacion; }

void Personaje::setPosicion(const Vector3& nuevaPosicion) { m_posicion = nuevaPosicion; }
void Personaje::setRotacion(const Vector3& nuevaRotacion) { m_rotacion = nuevaRotacion; }

std::vector<EstadoPersonaje> Personaje::getEstados() const
{
    return m_estadosActuales;
}

DatosRenderPersonaje Personaje::obtenerDatosParaGraficar()
{
    return DatosRenderPersonaje 
    {
        m_id,
        m_posicion,
        m_rotacion,
        m_estadosActuales
    };
}

