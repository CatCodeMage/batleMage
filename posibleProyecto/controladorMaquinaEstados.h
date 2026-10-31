#ifndef CONTROLADOR_MAQUINA_ESTADOS_H
#define CONTROLADOR_MAQUINA_ESTADOS_H
#include "controladorPersonaje.h"
#include "personaje.h"

struct InfoHechizo {
    int id;
    float costeMana;
    float alcance; //distancia max
    float cooldown;
    float velocidad;
};

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

        std::vector<InfoHechizo> m_catalogoHechizos;

        std::vector<InfoHechizo> m_hechizosLanzables;

        float m_distanciaSegura;
        float m_velocidadMovimiento;
        float m_cooldownAtaque;
        float m_tiempoDesdeUltAtaque;
        float m_regenMana;

        void evaluarTransiciones(const Personaje& personaje, float distObjetivo);
        void ejecutarEstado(Personaje& personaje, const glm::vec3& direccionHaciaObjetivo, float distancia, float deltaTime);
        void apuntarHacia(Personaje& personaje, const glm::vec3& direccion);
        void acercarseAlObjetivo(Personaje& personaje, const glm::vec3& direccionHaciaObjetivo, float deltaTime);

        float m_sentidoRodeo = 1.0f;             
        float m_tiempoHastaCambiarSentido = 0.0f;
        void rodearObjetivo(Personaje& personaje, const glm::vec3& direccionHaciaObjetivo, float deltaTime);

        glm::vec3 m_posicionAnteriorObjetivo{ 0.0f };
        bool      m_conocePosicionAnteriorObjetivo = false;
        glm::vec3 m_velocidadObjetivo{ 0.0f };
        glm::vec3 calcularDireccionAnticipada(const Personaje& personaje, float velocidadHechizo) const;

    public:
        ControladorMaquinaEstados(
            Personaje* objetivo = nullptr,
            const std::vector<InfoHechizo>& catalogoHechizos = {},
            float distanciaSegura = 5.0f,
            float velocidadMovimiento = 4.0f,
            float cooldownAtaque = 1.5f,
            float regeneracionManaSeg = 3.0f
        );
        void cargarCatalogoDesdeGestor(const HechizosGestor& gestor);

        virtual ~ControladorMaquinaEstados() = default;

        void actualizar(Personaje& personaje, float deltaTime) override;
        void setcatalogoHechizos(const std::vector<InfoHechizo>& catalogo);
        void setObjetivo(Personaje* nuevoObjetivo);
        Personaje* getObjetivo() const;
        EstadoIA getEstadoIA() const;
};
#endif