#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <memory>
#include <cstdint>   // NUEVO: uint32_t
#include <array>     // NUEVO: std::array
#include <utility>   // NUEVO: std::move
#include "global.h" 
#include "CombatTypes.h"

enum class CastType { Projectile, AreaOfEffect, Cone, Raycast, Self };
enum class MovementPattern { Linear, Homing, Parabolic, Orbiting, Static };

// 2. El componente aloja el enum junto al resto de datos estáticos
// he sacado del anterior struct los que cambia
struct SpellDefinitionComponent {
    int id;
    std::string name;
    ElementType element;
    CastType casteo;
    MovementPattern formaMovimiento;
    float cost;
    float damage;
    float cooldown;
};

// Componentes de estado dinámico
struct ProjectileComponent {
    glm::vec3 translacio;
    glm::vec3 rotacio;
    glm::vec3 scale;
    glm::vec3 direction;
    float speed;
    float radius;
};

// Comportamientos que puede activar un modificador (bits combinables)
enum EtiquetaHechizo : uint32_t {
    ETQ_NINGUNA = 0,
    ETQ_REBOTE = 1u << 0,
    ETQ_HOMING = 1u << 1,   // OPCIONAL: reservado, aún no se usa
};

// "Hoja de ajustes" de un disparo. Valores por defecto = sin cambios.
// Regla de orden: los extras se SUMAN, los multiplicadores se MULTIPLICAN,
// y al aplicarlo al proyectil: (base + extra) * mult
struct ModificadoresDisparo {
    float    danoExtra = 0.0f;
    float    multVelocidad = 1.0f;
    float    lifetimeExtra = 0.0f;
    float    dispersionExtra = 0.0f;   // grados extra de separación en multicast
    int      rebotes = 0;
    uint32_t etiquetas = ETQ_NINGUNA;

    ModificadoresDisparo& operator+=(const ModificadoresDisparo& o) {
        danoExtra += o.danoExtra;
        multVelocidad *= o.multVelocidad;
        lifetimeExtra += o.lifetimeExtra;
        dispersionExtra += o.dispersionExtra;
        rebotes += o.rebotes;
        etiquetas |= o.etiquetas;
        return *this;
    }
};

// Máximo de proyectiles en un mismo disparo (multicast). Array fijo: sin heap al disparar.
constexpr int MAX_PROYECTILES_DISPARO = 16;

// El "paquete" que se monta mientras se leen las cartas de la varita
struct EstadoDisparo {
    ModificadoresDisparo mods;
    std::array<int, MAX_PROYECTILES_DISPARO> proyectiles{};   // ids de cartas proyectil
    int   numProyectiles = 0;
    float mana = 0.0f;
    float castDelay = 0.0f;
    float recarga = 0.0f;

    bool anadirProyectil(int id) {
        if (numProyectiles >= MAX_PROYECTILES_DISPARO) return false;
        proyectiles[numProyectiles++] = id;
        return true;
    }
};

//pensado para IA / interfaz / json
enum class TipoCarta { Proyectil, Modificador, Multicast };

// Cuántas cartas quedan por robar en este disparo
struct LectorCartas {
    int robosPendientes = 1;   // la varita dispara 1 carta por defecto
};

// Lo común a todas las cartas. Cada tipo implementa aplicar() (como la action() de Noita)
class Carta {
public:
    Carta(int id, std::string nombre, float mana, float castDelay, float recarga = 0.0f)
        : m_id(id), m_nombre(std::move(nombre)), m_mana(mana), m_castDelay(castDelay), m_recarga(recarga) {
    }
    virtual ~Carta() = default;

    virtual TipoCarta tipo() const = 0;   // OPCIONAL (va junto a TipoCarta)
    virtual void aplicar(EstadoDisparo& estado, LectorCartas& lector) const = 0;

    int getId() const { return m_id; }
    const std::string& getNombre() const { return m_nombre; }
    float getMana() const { return m_mana; }
    float getCastDelay() const { return m_castDelay; }
    float getRecarga() const { return m_recarga; }

protected:
    int m_id;
    std::string m_nombre;
    float m_mana;
    float m_castDelay;
    float m_recarga;
};

// Sus stats (daño, velocidad...) siguen en el catálogo de HechizosGestor (hechizosTemplate)
class CartaProyectil : public Carta {
public:
    using Carta::Carta;
    TipoCarta tipo() const override { return TipoCarta::Proyectil; }  
    void aplicar(EstadoDisparo& e, LectorCartas& l) const override {
        e.anadirProyectil(m_id);
        l.robosPendientes--;            // gasta un robo
    }
};

class CartaModificador : public Carta {
public:
    CartaModificador(int id, std::string nombre, float mana, float castDelay, const ModificadoresDisparo& mods)
        : Carta(id, std::move(nombre), mana, castDelay), m_mods(mods) {
    }
    TipoCarta tipo() const override { return TipoCarta::Modificador; }  
    void aplicar(EstadoDisparo& e, LectorCartas&) const override {
        e.mods += m_mods;               // no gasta robo -> "roba otra carta"
    }
    const ModificadoresDisparo& getMods() const { return m_mods; }
private:
    ModificadoresDisparo m_mods;
};

class CartaMulticast : public Carta {
public:
    CartaMulticast(int id, std::string nombre, float mana, float castDelay, int robos)
        : Carta(id, std::move(nombre), mana, castDelay), m_robos(robos) {
    }
    TipoCarta tipo() const override { return TipoCarta::Multicast; }    
    void aplicar(EstadoDisparo&, LectorCartas& l) const override {
        l.robosPendientes += m_robos - 1;   // "doble": 1 -> 2
    }
    int getRobos() const { return m_robos; }
private:
    int m_robos;
};