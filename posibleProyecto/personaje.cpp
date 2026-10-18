#include "personaje.h"
#include <iostream>
using namespace std;

Personaje::Personaje(int id, Vector3 posicionInicial, float radio)
{
    this->m_id = id;
    this->m_posicion = posicionInicial;
    this->m_rotacion = Vector3(1.0f,1.0f,1.0f);
    this->m_radio = radio;
    this->m_margenCasteo = 0.5f;
    this->m_vida = 100.0f;
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
    if(yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO))return;
    if(m_controlador != nullptr)
    {
        m_controlador->actualizar(*this, deltaTime);
    }
}

void Personaje::moverse(const Vector3& desplazamiento)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO)) return;

    m_posicion = m_posicion + desplazamiento;
    
    borrarEstado(EstadoPersonaje::IDLE);
    anadirEstado(EstadoPersonaje::VOLANDO);
}

void Personaje::rotar(const Vector3& rotacion)
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO)) return;

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

void Personaje::lanzarHechizo(int idHechizo) //tener en cuenta que puede lanzar cualquier hechizo no tiene restricciones, enviar a hechizo q hechizo se manda y tal
                                //para enviarle a hechizo le tengo que pasar la posicion donde se origina que es la posicion del centro de la 
                                //bola mas el grosor de la bola mas un espacio para que no salga pegado al personaje
                                //no tener en cuenta si hay un obstaculo en el origen del hechizo pq se comunicara con hechizo
                                //si hay un obstaculo hechizo se comunica con histbox y se estampe el hechizo con el obstaculo directamente
                                //le pasamos el id del hechizo que el personaje puede tirar cualquier id de hechizo y ahi hechizo sabra que hechizo es
{
    if (yaTieneEstado(EstadoPersonaje::MUERTO) || yaTieneEstado(EstadoPersonaje::CONGELADO)) return;
    
    anadirEstado(EstadoPersonaje::CASTEANDO);
    
    //direccion normalizada donde mira el pj
    float longitud = std::sqrt(m_rotacion.x * m_rotacion.x + m_rotacion.y * m_rotacion.y + m_rotacion.z * m_rotacion.z);

    Vector3 direccionDisparo(1.0f, 0.0f, 0.0f); //por defecto si rotacion es 0,0,0
    if(longitud > 0.0001f)
    {
        direccionDisparo = Vector3(m_rotacion.x / longitud, m_rotacion.y / longitud, m_rotacion.z / longitud);
    }
    float distanciaSeparacion = m_radio + m_margenCasteo;

    Vector3 posicionOrigenHechizo(
        m_posicion.x + direccionDisparo.x * distanciaSeparacion,
        m_posicion.y + direccionDisparo.y * distanciaSeparacion,
        m_posicion.z + direccionDisparo.z * distanciaSeparacion
    );
    cout << "Personaje " << m_id << " lanza el hechizo ID [" << idHechizo << "] desde origen (" << posicionOrigenHechizo.x
        <<", " << posicionOrigenHechizo.y << ", " << posicionOrigenHechizo.z << ")" << endl;
    
    //Comunicación con modulo hechizos
}

void Personaje::setControlador(ControladorPersonaje* nuevoControlador)
 {
    m_controlador = nuevoControlador;
}

int Personaje::getId() const { return m_id; }

float Personaje::getVida() const { return m_vida; }

float Personaje::getMana() const { return m_mana; }

float Personaje::getRadio() const { return m_radio; }

Vector3 Personaje::getPosicion() const { return m_posicion; }
Vector3 Personaje::getRotacion() const { return m_rotacion; }

void Personaje::setPosicion(const Vector3& nuevaPosicion) { m_posicion = nuevaPosicion; }
void Personaje::setRotacion(const Vector3& nuevaRotacion) { m_rotacion = nuevaRotacion; }
void Personaje::setRadio(float radio) { m_radio = radio; }

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
        m_radio,
        m_estadosActuales
    };
}

