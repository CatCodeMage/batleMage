#ifndef CONTROLADOR_MAQUINA_ESTADOS
#define CONTROLADOR_MAQUINA_ESTADOS_H
#include "controladorPersonaje.h"
#include "personaje.h"

enum class EstadoIA 
{
    ESPERANDO,
    PERSIGUIENDO,
    ATACANDO,
    HUYENDO
};

class ControladorMaquinaEstados : public ControladorPersonaje 
{
    private:
        Personaje* m_objetivo;
        EstadoIA m_estadoIA;

        float m_distanciaAtaque;
        float m_distanciaSegura;
        float m_velocidadMovimiento;
        float m_costeManaHechizo;
        float m_cooldownAtaque;
        float m_tiempoDesdeUltAtaque;
        float m_regenMana;

        void evaluarTransiciones(const Personaje& personaje, float distObjetivo);
        void ejecutarEstado(Personaje& personaje, const Vector3& direccionHaciaObjetivo, float distancia, float deltaTime);
        void apuntarHacia(Personaje& personaje, const Vector3& direccion);

    public:
        ControladorMaquinaEstados(
            Personaje* objetivo = nullptr,
            float distanciaAtaque = 15.0f,
            float distanciaSegura = 5.0f,
            float velocidadMovimiento = 4.0f,
            float costeManaHechizo = 10.0f,
        float cooldownAtaque = 1.5f,
        float regeneracionManaSeg = 3.0f
    );

    virtual ~ControladorMaquinaEstados() = default;

    void actualizar(Personaje& personaje, float deltaTime) override;

    void setObjetivo(Personaje* nuevoObjetivo);
    Personaje* getObjetivo() const;
    EstadoIA getEstadoIA() const;

};
#endif