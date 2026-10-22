#pragma once

#include <glm/glm.hpp>
#include <variant>

#include "luz.h"

// Tipos de objetos que pueden aparecer en el escenario.
enum class tipoAsset
{
    ARBOL
};

// Cada tipo de asset puede tener sus propios estados.
enum class estadoArbol
{
    ENTERO,
    DESTRUIDO
};

using estadoAsset = std::variant<estadoArbol>;

class Assets
{
public:
    // Compatible con las dos formas del constructor original (con/sin posición).
    Assets(tipoAsset tipo, estadoAsset estado,
           glm::vec3 coordenadas = glm::vec3(0.0f));
    ~Assets() = default;

    // Consultas básicas.
    estadoAsset getEstado() const;
    tipoAsset getTipo() const;
    glm::vec3 getPosicion() const;
    glm::vec3 getRotacion() const;
    glm::vec3 getEscala() const;
    bool esDestruible() const;

    // Modificación del estado y de las transformaciones.
    void setEstado(estadoAsset estado);
    void setCoordenadas(glm::vec3 coordenadas);
    void setRotacion(glm::vec3 rotacion);
    void setEscala(glm::vec3 escala);
    void setDestruible(bool destruible);

    // Desplazamiento relativo en coordenadas del mundo.
    void trasladar(glm::vec3 desplazamiento);
    void incX(float incremento);
    void incY(float incremento);
    void incZ(float incremento);

    // Iluminación. No hay fuente de luz por defecto.
    bool tieneFuenteDeLuz() const;
    // Devuelve nullptr si el asset no emite luz.
    const FuenteDeLuz* getFuenteDeLuz() const;
    void setFuenteDeLuz(const FuenteDeLuz& fuente);
    void quitarFuenteDeLuz();

private:
    tipoAsset m_tipo;
    estadoAsset m_estado;

    glm::vec3 m_coordenadas{0.0f};
    glm::vec3 m_rotacion{0.0f}; // Ángulos en grados (X, Y, Z).
    glm::vec3 m_escala{1.0f};
    bool m_destruible{false};

    bool m_tieneFuenteDeLuz{false};
    FuenteDeLuz m_fuenteDeLuz{};
};
