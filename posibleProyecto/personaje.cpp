#include "personaje.h"
#include <iostream>
using namespace std;

Personaje::Personaje(int id, float x, float y, float z)
{
    this->m_id = id;
    this->m_x = x;
    this->m_y = y;
    this->m_z = z;
    this->m_vida = 100.f;
    this->m_mana = 50.0f;
    this->m_estadoActual = EstadoPersonaje::IDLE;
    m_controlador = nullptr;
}

void Personaje::update(float deltaTime)
{
    if(m_estadoActual == EstadoPersonaje::MUERTO) return;
    if(m_controlador != nullptr)
    {
        m_controlador->actualizar(*this, deltaTime);
    }
}

void Personaje::moverse(float deltaX, float deltaY, float deltaZ)
{
    m_x += deltaX;
    m_y += deltaY;
    m_z += deltaZ;
    m_estadoActual = EstadoPersonaje::VOLANDO;
}

void Personaje::recibirDaño(float cantidadDaño)
{
    if(m_estadoActual == EstadoPersonaje::MUERTO) return;
    m_vida -= cantidadDaño;
    cout << "Personaje " << m_id << " recibe " << cantidadDaño << " de daño. Vida restante: " << m_vida << endl;

    if (m_vida <= 0)
    {
        m_vida = 0;
        m_estadoActual = EstadoPersonaje::MUERTO;
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
    m_estadoActual = EstadoPersonaje::CASTEANDO;
    cout << "Personaje " << m_id << " lanxa un hechizo en la posicion (" << m_x<< "," << m_y << "," << m_z << ")" << endl;
    //pasar projectil a la logica de colisiones
}

void Personaje::setControlador(ControladorPersonaje* nuevoControlador)
 {
    m_controlador = nuevoControlador;
}

void Personaje::setEstado(EstadoPersonaje nuevoEstado) 
{
    m_estadoActual = nuevoEstado;
}

int Personaje::getId() const { return m_id; }

float Personaje::getVida() const { return m_vida; }

float Personaje::getMana() const { return m_mana; }

float Personaje::getX() const { return m_x; }

float Personaje::getY() const { return m_y; }

float Personaje::getZ() const { return m_z; }

EstadoPersonaje Personaje::getEstado() const { return m_estadoActual; }

DatosRenderPersonaje Personaje::obtenerDatosParaGraficar()
{
    return DatosRenderPersonaje 
    {
        m_id,
        m_x, m_y, m_z,
        m_estadoActual
    };
}

