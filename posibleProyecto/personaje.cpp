#include "personaje.h"
#include <iostream>
using namespace std;

Personaje::Personaje(int id, float x, float y, float z)
{
    this->m_id = id;
    this->m_x = x;
    this->m_y = y;
    this->m_z = z;
    this->m_rotX = 0.0f;
    this->m_rotY = 0.0f;
    this->m_rotZ = 0.0f;
    this->m_vida = 100.f;
    this->m_mana = 50.0f;
    añadirEstado(EstadoPersonaje::IDLE);
    m_controlador = nullptr;
}

void Personaje::añadirEstado(EstadoPersonaje nuevoEstado)
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

void Personaje::moverse(float deltaX, float deltaY, float deltaZ)
{
    if((yaTieneEstado(EstadoPersonaje::MUERTO)) ||(yaTieneEstado(EstadoPersonaje::PARALIZADO)) )return;
    m_x += deltaX;
    m_y += deltaY;
    m_z += deltaZ;
    borrarEstado(EstadoPersonaje::IDLE);
    añadirEstado(EstadoPersonaje::VOLANDO);
}

void Personaje::rotar(float deltaRotX, float deltaRotY, float deltaRotZ)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::PARALIZADO)) return;

    m_rotX += deltaRotX;
    m_rotY += deltaRotY;
    m_rotZ += deltaRotZ;
}

void Personaje::recibirDaño(float cantidadDaño)
{
    if(yaTieneEstado(EstadoPersonaje::MUERTO)) return;
    m_vida -= cantidadDaño;
    cout << "Personaje " << m_id << " recibe " << cantidadDaño << " de daño. Vida restante: " << m_vida << endl;

    if (m_vida <= 0)
    {
        m_vida = 0;
        m_estadosActuales.clear();
        añadirEstado(EstadoPersonaje::MUERTO);
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

    añadirEstado(EstadoPersonaje::CASTEANDO);
    cout << "Personaje " << m_id << " lanxa un hechizo en la posicion (" << m_x<< "," << m_y << "," << m_z << ")" << endl;
    //pasar projectil a la logica de colisiones
}

void Personaje::setControlador(ControladorPersonaje* nuevoControlador)
 {
    m_controlador = nuevoControlador;
}

int Personaje::getId() const { return m_id; }

float Personaje::getVida() const { return m_vida; }

float Personaje::getMana() const { return m_mana; }

float Personaje::getX() const { return m_x; }

float Personaje::getY() const { return m_y; }

float Personaje::getZ() const { return m_z; }

std::vector<EstadoPersonaje> Personaje::getEstados() const
{
    return m_estadosActuales;
}

DatosRenderPersonaje Personaje::obtenerDatosParaGraficar()
{
    return DatosRenderPersonaje 
    {
        m_id,
        m_x, m_y, m_z,
        m_rotX, m_rotY, m_rotZ,
        m_estadosActuales
    };
}

