#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "controladorPersonaje.h"
#include <memory>
#include <vector>

enum class EstadoPersonaje {
    IDLE,      //en la esfera esta siempre en idle
    VOLANDO,    
    CASTEANDO, 
    MUERTO,
    QUEMADO,
    PARALIZADO,
    ENVENENADO
    //se pueden meter mas
};

struct DatosRenderPersonaje
{
    int idPersonaje;
    float posX, posY, posZ;
    float rotX, rotY, rotZ;
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
    float m_x, m_y, m_z;
    float m_rotX, m_rotY, m_rotZ;
    std::vector<EstadoPersonaje> m_estadosActuales;
    ControladorPersonaje* m_controlador;

public:
    Personaje(int id, float x, float y, float z);
    void update(float deltaTime);

    void moverse(float deltaX, float deltaY, float deltaZ);
    void rotar(float deltaRotX, float deltaRotY, float deltaRotZ);
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
    float getX() const;
    float getY() const;
    float getZ() const;
    EstadoPersonaje getEstado() const;
    void setEstado(EstadoPersonaje nuevoEstado);
    void setControlador(ControladorPersonaje* nuevoControlador);

    DatosRenderPersonaje obtenerDatosParaGraficar();
};

#endif 
