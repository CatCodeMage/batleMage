#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "controladorPersonaje.h"
#include <memory>

enum class EstadoPersonaje {
    IDLE,      
    VOLANDO,    
    CASTEANDO, 
    MUERTO      
};

struct DatosRenderPersonaje
{
    int idPersonaje;
    float posX, posY, posZ;
    EstadoPersonaje estadoActual;
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
    EstadoPersonaje m_estadoActual;
    ControladorPersonaje* m_controlador;

public:
    Personaje(int id, float x, float y, float z);
    void update(float deltaTime);

    void moverse(float deltaX, float deltaY, float deltaZ);
    void lanzarHechizo();
    void recibirDaño(float cantidadDaño);
    void aumentarVida(float cantidad);
    void disminuirMana(float cantidad);
    void aumentarMana(float cantidad);

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