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
    PARALIZADO,
    CONGELADO,
};

struct DatosRenderPersonaje
{
    int idPersonaje;
    Vector3 m_posicion;
    Vector3 m_rotacion;
    std::vector<EstadoPersonaje> m_estadosActuales;
};

class Personaje
{
private:
    int m_id;
    float m_vida;
    float m_mana; 
    float m_capacidadMagia; //estadistica del personaje como las del noita 
                        //(ej: veloc recarga mana, num hechizos lanzar a la vez...)
    Vector3 m_posicion;
    Vector3 m_rotacion;
    std::vector<EstadoPersonaje> m_estadosActuales;
    ControladorPersonaje* m_controlador;

public:
    Personaje(int id, Vector3 posicionInicial);
    void update(float deltaTime);

    void moverse(const Vector3& desplazamiento);
    void rotar(const Vector3& deltaRotacion);
    void lanzarHechizo();
    void recibirDaño(float cantidadDaño);
    void aumentarVida(float cantidad);
    void disminuirMana(float cantidad);
    void aumentarMana(float cantidad);

    void añadirEstado(EstadoPersonaje nuevoEstado);
    void borrarEstado(EstadoPersonaje estado);
    bool yaTieneEstado(EstadoPersonaje estado);
    std::vector<EstadoPersonaje> getEstados() const;

    int getId() const;
    float getVida() const;
    float getMana() const;
    Vector3 getPosicion() const;
    Vector3 getRotacion() const;
    void setPosicion(const Vector3& nuevaPosicion);
    void setRotacion(const Vector3& nuevaRotacion);
    EstadoPersonaje getEstado() const;
    void setEstado(EstadoPersonaje nuevoEstado);
    void setControlador(ControladorPersonaje* nuevoControlador);

    DatosRenderPersonaje obtenerDatosParaGraficar();
};

#endif 
