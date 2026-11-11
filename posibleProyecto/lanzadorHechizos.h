#pragma once
#include <vector>
#include "hechizoDatos.h"   // EstadoDisparo, glm

// Declaración adelantada: solo guardamos una referencia, no hace falta el header completo
class HechizosGestor;

// Lo que la varita necesita saber después de disparar
struct ResultadoDisparo {
    bool  disparado = false;
    int   proyectilesLanzados = 0;
    float manaGastado = 0.0f;
    float castDelay = 0.0f;   // se SUMA al castDelay de la varita
    float recarga = 0.0f;   // se SUMA al rechargeTime de la varita
    int   nuevoIndice = 0;
    bool  hayQueRecargar = false;
};

// Puente con la varita: lee las cartas de izquierda a derecha y lanza el disparo.
// m_gestor es una REFERENCIA al gestor que ya existe (no una copia).
class LanzadorHechizos {
public:
    explicit LanzadorHechizos(HechizosGestor& gestor) : m_gestor(gestor) {}

    // Solo lee cartas y monta el paquete (no crea nada). Útil para pruebas y para la IA.
    EstadoDisparo leerCartas(const std::vector<int>& ranuras, int& indice, float manaDisponible) const;

    // Lee + lanza. Es lo único que necesita llamar la varita.
    ResultadoDisparo disparar(const std::vector<int>& ranuras, int indiceInicial, float manaDisponible,
        const glm::vec3& origen, const glm::vec3& direccion, int idLanzador);
private:
    HechizosGestor& m_gestor;
};