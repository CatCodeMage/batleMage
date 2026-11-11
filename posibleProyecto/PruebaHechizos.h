#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include "personaje.h"
#include "hechizosGestor.h"
#include "Collision.h"
#include "CombatManager.h"

// ---------------------------------------------------------------------
// COMBATE DE PRUEBA SOLO PARA HECHIZOS (no sustituye a CombatSimulation).
// Dos magos quietos y sin IA que disparan con "varitas de prueba" usando
// LanzadorHechizos. Genera el replay en el mismo formato que CombatSimulation,
// así los gráficos lo reproducen sin cambios.
// Tiene la misma interfaz que usa main.cpp: setup, step, isFinished,
// printResult, recordEnding y writeReplay.
// ---------------------------------------------------------------------
class PruebaHechizos {
public:
    PruebaHechizos();

    void setup(const std::filesystem::path& combatConfigFile);
    void step();
    bool isFinished() const;
    void printResult() const;
    void recordEnding();
    void writeReplay(const std::filesystem::path& replayFile) const;

    static constexpr int   TICKS_PER_SECOND = 60;
    static constexpr float SECONDS_PER_TICK = 1.0f / TICKS_PER_SECOND;

private:
    // Hace el papel de la Varita mientras la de verdad no esté conectada
    struct VaritaPrueba {
        std::vector<int> ranuras;        // ids de cartas, de izquierda a derecha
        int   indice = 0;
        float espera = 0.0f;       // cast delay / recarga pendiente
        float mana = 200.0f;
        float manaMax = 200.0f;
        float regenMana = 40.0f;      // por segundo
        float castDelay = 0.3f;
        float recarga = 1.2f;
        float inclinacionY = 0.0f;       // > 0 hacia arriba, < 0 hacia abajo
    };

    void registrarCartas();
    void crearMagos(const std::filesystem::path& combatConfigFile);
    void crearVaritas();
    void dispararVaritas();
    void aplicarGravedad();
    void aplicarSuelo();
    void aplicarImpactos();
    void countDownHitEffects();
    void recordFrame();

    int         idPorNombre(const std::string& nombre) const;
    size_t      countAlive() const;
    std::string describeWinner() const;
    const char* replayEntityOfPlayer(size_t playerIndex) const;

    std::vector<std::unique_ptr<Personaje>> m_players;
    std::vector<VaritaPrueba> m_varitas;          // m_varitas[i] es la de m_players[i]
    std::vector<int>   m_hitFrames;
    std::vector<int>   m_impactosRecibidos;
    std::vector<float> m_danoRecibido;
    int m_disparos = 0;
    int m_proyectiles = 0;

    HechizosGestor   m_spells;
    CombatManager    m_combat;
    CollisionManager m_colisiones;
    int m_tick = 0;

    nlohmann::json m_replayFrames = nlohmann::json::array();
};