#pragma once
// CONTRATO entre Hechizos (crea los efectos) y Colisiones/Combate (los aplica).
// Añadir campos o formas es seguro; cambiar o quitar algo hay que hablarlo.
#include <algorithm>
#include <variant>
#include <vector>
#include "hechizoDatos.h"   // ElementType, glm

// ---------- Formas (cada una lleva solo sus medidas; tamaño final) ----------
// Todas se colocan en 'posicion' y miran hacia 'direccion' (en el plano del suelo).
// En vertical ocupan una franja centrada en posicion.y.
struct Esfera { float radio = 0.0f; };
struct Cono   { float alcance = 0.0f; float angulo = 60.0f; float altura = 100.0f; };   // cuña: sector + grosor
struct Caja   { float ancho = 0.0f; float alto = 100.0f; float largo = 0.0f; };       // empieza en posicion y se alarga 'largo' hacia delante

using FormaZona = std::variant<std::monostate, Esfera, Cono, Caja>;   // monostate = sin zona

// ---------- Datos fijos de la zona (JSON -> catálogo) ----------
struct DatosZona {
    FormaZona forma;                 // por defecto: sin zona
    float escalaInicial = 1.0f;      // tamaño al aparecer respecto al final (crece radio / alcance / largo)
    float duracion = 0.0f;           // segundos que dura / crece; 0 = instantánea
    float intervaloDano = 0.0f;      // 0 = cada mago recibe daño 1 vez; >0 = daño cada X s mientras esté dentro
    bool tieneZona() const { return !std::holds_alternative<std::monostate>(forma); }
};

// ---------- Qué le hace al objetivo ----------
// Crece aquí (empuje, estado, escudo…) sin tocar zonas ni bucles.
struct CargaEfecto {
    int idHechizo = -1;                           // para nombre, modelo, etc. vía catálogo
    float dano = 0.0f;                            // ya con modificadores; negativo = cura
    ElementType elemento = ElementType::Physical;
};

// ---------- Entregas que colisiones/combate resuelve ----------
struct EfectoZona {
    DatosZona datos;
    glm::vec3 posicion{ 0.0f };
    glm::vec3 direccion{ 0.0f, 0.0f, 1.0f };
    CargaEfecto carga;
    int idLanzador = -1;
    float tiempo = 0.0f;               // segundos activo (lo avanza HechizosGestor)
    float proximoGolpe = 0.0f;         // solo si intervaloDano > 0 (lo avanza colisiones)
    std::vector<int> yaGolpeados;      // ids ya golpeados (solo si intervaloDano == 0)

    float escalaActual() const {
        const float t = datos.duracion > 0.0f ? std::min(tiempo / datos.duracion, 1.0f) : 1.0f;
        return datos.escalaInicial + (1.0f - datos.escalaInicial) * t;
    }
    bool terminado() const { return tiempo > datos.duracion; }   // dura al menos 1 tick
    bool haGolpeado(int id) const {
        return std::find(yaGolpeados.begin(), yaGolpeados.end(), id) != yaGolpeados.end();
    }
    void marcarGolpeado(int id) { if (!haGolpeado(id)) yaGolpeados.push_back(id); }
};

struct EfectoSobreLanzador {
    CargaEfecto carga;
    int idLanzador = -1;
};
