#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "controladorPersonaje.h"
#include <memory>
#include <vector>
#include "hechizoDatos.h"

enum class EstadoPersonaje {
    IDLE,      //en la esfera esta siempre en idle
    VOLANDO,    
    CASTEANDO, 
    MUERTO,
    QUEMADO,
    CONGELADO,
};

struct DatosRenderPersonaje
{
    int idPersonaje;
    Vector3 m_posicion;
    Vector3 m_rotacion;
    float m_radio;
    std::vector<EstadoPersonaje> m_estadosActuales;
};

class Personaje //personaje es bolita pasar datos de cuanto mide y tal
{
private:
    int m_id;
    float m_vida;
    float m_vidaMax;
    float m_mana;
    float m_manaMax; 
    float m_capacidadMagia; //estadistica del personaje como las del noita 
                        //(ej: veloc recarga mana, num hechizos lanzar a la vez...)
    float m_tiempoQuemado;
    float m_tiempoCongelado;
    float m_danoQuemadura;
    Vector3 m_posicion;
    Vector3 m_rotacion;
    float m_radio;
    float m_margenCasteo;
    std::vector<EstadoPersonaje> m_estadosActuales;
    ControladorPersonaje* m_controlador;

public:
    Personaje(int id, Vector3 posicionInicial, float radio = 1.0f);
    void update(float deltaTime);

    void moverse(const Vector3& desplazamiento);
    void rotar(const Vector3& deltaRotacion);
    void lanzarHechizo(int idHechizo);
    void recibirDano(float cantidadDano);
    void aumentarVida(float cantidad);
    void disminuirMana(float cantidad);
    void aumentarMana(float cantidad);

    void anadirEstado(EstadoPersonaje nuevoEstado, float duracion = 3.0f);
    void borrarEstado(EstadoPersonaje estado);
    bool yaTieneEstado(EstadoPersonaje estado);
    std::vector<EstadoPersonaje> getEstados() const;

    int getId() const;
    float getVida() const;
    float getMana() const;
    float getRadio() const;
    Vector3 getPosicion() const;
    Vector3 getRotacion() const;
    void setPosicion(const Vector3& nuevaPosicion);
    void setRotacion(const Vector3& nuevaRotacion);
    void setRadio(float radio);
    void setControlador(ControladorPersonaje* nuevoControlador);

    DatosRenderPersonaje obtenerDatosParaGraficar();
};
#endif 
