#pragma once

#include <cstdint>
#include <glm/glm.hpp>
#include <vector>

#include "luz.h"

// Configuración visual del escenario.
enum class skybox
{
    CIELO_AZUL
};

enum class tipoMapa
{
    BASE
};

enum class tipoMarca
{
    FUEGO,
    HIELO
};

struct marca
{
    glm::vec3 coordenadas{0.0f};
    float radio{0.0f};
    tipoMarca tipo{tipoMarca::FUEGO};
    std::uint64_t tickCreacion{0};
};

class Map
{
public:
    // Dimensiones = (ancho X, alto Y, largo Z).
    // El mapa está centrado en X=0, Z=0 y comienza en Y=alturaSuelo.
    Map(tipoMapa tipo, glm::vec3 dimensiones, skybox cielo,
        LlumAmbient luz, float alturaSuelo = 0.0f);
    ~Map() = default;

    // Consultas.
    tipoMapa getTipo() const;
    glm::vec3 getDimensiones() const;
    const std::vector<marca>& getMarcas() const;
    skybox getSkyBox() const;
    LlumAmbient getLuzAmbiental() const;
    float getAlturaSuelo() const;

    // Configuración ambiental.
    void setColorLlumAmbient(glm::vec3 rgb);
    void setIntensitatLlumAmbient(float intensidad);
    // Se conserva el nombre original para no romper llamadas existentes.
    void setInsensitatLlumAmbient(float intensidad);
    void setSkyBox(skybox cielo);
    void setAlturaSuelo(float alturaSuelo);

    // Marcas provocadas por hechizos.
    void addMarca(const marca& nuevaMarca);
    void clearMarcas();

    // Límites y posiciones (sin tener en cuenta hitboxes).
    // Comprueba solo la superficie horizontal del mapa (ejes X y Z).
    bool estaDentroDelMapa(glm::vec3 posicion) const;
    // Comprueba X, Z y también la altura Y entre suelo y techo.
    bool esPosicionValida(glm::vec3 posicion) const;
    // Restringe la posición a los límites del volumen del mapa.
    glm::vec3 limitarPosicion(glm::vec3 posicion) const;

    // Restaura el ambiente y la altura iniciales y elimina las marcas.
    void reiniciarMapa();

private:
    tipoMapa m_tipo;
    glm::vec3 m_dimensiones;
    float m_alturaSuelo;
    std::vector<marca> m_marcas;

    skybox m_skybox;
    LlumAmbient m_llumAmbient;

    // Configuración guardada para reiniciar el escenario.
    skybox m_skyboxInicial;
    LlumAmbient m_llumAmbientInicial;
    float m_alturaSueloInicial;
};
