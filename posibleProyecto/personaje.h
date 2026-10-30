#ifndef PERSONAJE_H
#define PERSONAJE_H

#include "controladorPersonaje.h"
#include <memory>
#include <vector>
#include "hechizoDatos.h"
#include <glm/glm.hpp>

// Personaje y la IA solo usan x, y, z y el constructor (x, y, z): glm::vec3 encaja tal cual
using Vector3 = glm::vec3;


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
    glm::vec3 m_posicion;
    glm::vec3 m_rotacion;
    float m_radio;
    std::vector<EstadoPersonaje> m_estadosActuales;
};

class Personaje
{
private:
    std::string m_tipoPersonaje;
    glm::vec3 m_escala;
    int m_id;
    float m_vida;
    float m_vidaMax;
    float m_mana;
    float m_manaMax; 
    float m_capacidadMagia; 
    float m_tiempoQuemado;
    float m_tiempoCongelado;
    float m_danoQuemadura;
    glm::vec3 m_posicion;
    glm::vec3 m_rotacion;
    glm::vec3 m_direccion;
    float m_radio;
    float m_margenCasteo;
    std::vector<EstadoPersonaje> m_estadosActuales;
    ControladorPersonaje* m_controlador;
    HechizosGestor* m_gestorHechizos;

public:
    Personaje(int id, glm::vec3 posicionInicial, float radio = 1.0f, HechizosGestor* gestorHechizos = nullptr);
    void update(float deltaTime);

    void moverse(const glm::vec3& desplazamiento);
    void rotar(const glm::vec3& deltaRotacion);
    void lanzarHechizo(int idHechizo);
    void recibirDano(float cantidadDano);
    void aumentarVida(float cantidad);
    void disminuirMana(float cantidad);
    void aumentarMana(float cantidad);

    void anadirEstado(EstadoPersonaje nuevoEstado, float duracion = 3.0f);
    void borrarEstado(EstadoPersonaje estado);
    bool yaTieneEstado(EstadoPersonaje estado) const;
    std::vector<EstadoPersonaje> getEstados() const;
    void aplicarEfecto(EffectType efecto, float valor, float duracion);


    int getId() const;
    float getVida() const;
    float getMana() const;
    float getRadio() const;
    glm::vec3 getPosicion() const;
    glm::vec3 getRotacion() const;
    glm::vec3 getDireccion() const;
    void setPosicion(const glm::vec3& nuevaPosicion);
    void setRotacion(const glm::vec3& nuevaRotacion);
    void setDireccion(const glm::vec3& nuevaDireccion);
    void setRadio(float radio);
    void setControlador(ControladorPersonaje* nuevoControlador);

    void setGestorHechizos(HechizosGestor* gestor);
    HechizosGestor* getGestorHechizos() const;
    std::string getEstadoPrincipalString() const;

    DatosRenderPersonaje obtenerDatosParaGraficar();

    bool cargarDesdeJson(const std::string& rutaArchivoPlayers, const std::string& tipoPersonaje);

    json exportarCabeceraJson() const; 
    json exportarFrameJson() const;
};
#endif 
