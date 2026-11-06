#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "hechizosGestor.h"

enum class TipoHechizo {
    LANZABLE,
    VARITA,
    MODIFICADOR_HECHIZO,
};
class Varita
{
private:
    std::string m_nombre;
    float m_castDelay;       
    float m_rechargeTime;      
    float m_manaMax;         
    float m_manaActual;      
    float m_recargaMana;   
    int m_capacidad;         
    float m_dispersion;      

    std::vector<int> m_ranuras; 
    int m_indiceActual;         
    float m_tiempoEspera;      

    glm::vec3 aplicarDispersion(const glm::vec3& direccionBase) const;
public:
    Varita(const std::string& nombre = "Varita basica",
           float castDelay = 0.20f,
           float rechargeTime = 0.80f,
           float manaMax = 100.0f,
           float recargaMana = 15.0f,
           int capacidad = 4,
           float dispersion = 2.0f);
    void update(float deltaTime);
    
    bool equiparHechizoEnRanura(int indiceRanura, int idHechizo);
    void limpiarRanura(int indiceRanura);

    bool intentarDisparar(const glm::vec3& posicionMago, float radioMago, float margenCasteo,
                          const glm::vec3& direccionApuntado, HechizosGestor* gestorHechizos);

    bool puedeAtacarObjetivo(float distanciaAlObjetivo, const HechizosGestor& gestor) const;
    float getVelocidadProximoHechizo(const HechizosGestor& gestor) const;

    void setNombre(const std::string& nombre) { m_nombre = nombre; }
    void setCastDelay(float delay) { m_castDelay = delay; }
    void setRechargeTime(float tiempo) { m_rechargeTime = tiempo; }
    void setManaMax(float manaMax) { m_manaMax = manaMax; m_manaActual = manaMax; }
    void setRecargaMana(float recarga) { m_recargaMana = recarga; }
    void setCapacidad(int capacidad) { m_capacidad = capacidad; m_ranuras.resize(capacidad); }
    void setDispersion(float dispersion) { m_dispersion = dispersion; }

    std::string getNombre() const { return m_nombre; }
    float getCastDelay() const { return m_castDelay; }
    float getRechargeTime() const { return m_rechargeTime; }
    float getManaMax() const { return m_manaMax; }
    float getManaActual() const { return m_manaActual; }
    float getRecargaMana() const { return m_recargaMana; }
    int getCapacidad() const { return m_capacidad; }
    float getDispersion() const { return m_dispersion; }
};

