#pragma once
#include <filesystem>
#include <memory>
#include <vector>
#include "personaje.h"
#include "controladorMaquinaEstados.h"
#include "hechizosGestor.h"
#include <string>
#include "CombatManager.h"
#include "jsonManager.h"   // readSpells
#include <cmath>
#include "Paths.h"

// Combate sin graficos: los personajes con su IA y los hechizos en vuelo.
// No toca OpenGL, para poder ejecutarse sin ventana.
class CombatSimulation {
public:
    CombatSimulation();
    void setup(const std::filesystem::path& combatConfigFile);
    void printState() const;
    void step();
    int  getTick() const { return m_tick; }

    // Paso fijo: el mismo combate da siempre el mismo resultado, vaya rapido o lento el ordenador
    static constexpr int   TICKS_PER_SECOND = 60;
    static constexpr float SECONDS_PER_TICK = 1.0f / TICKS_PER_SECOND;
    static constexpr int   MAX_COMBAT_SECONDS = 300;

    bool isFinished() const;
    int  getWinnerId() const;    
    void printResult() const;

    void writeReplay(const std::filesystem::path& replayFile) const;
private:
    void recordFrame();
    std::string describeWinner() const;
    void createPlayers(const std::filesystem::path& combatConfigFile);
    void connectOpponents();
    size_t opponentIndexOf(size_t playerIndex) const;
     
    void printSpellsInFlight() const;

    void applySpellHits();
    bool tryHitAnyPlayer(Hechizo& spell);
    size_t countAlivePlayers() const;

    // unique_ptr: la IA guarda un puntero a su rival y el personaje uno a su IA,
    // asi que sus direcciones no pueden cambiar cuando el vector crece
    std::vector<std::unique_ptr<Personaje>> m_players;
    std::vector<std::unique_ptr<ControladorMaquinaEstados>> m_controllers;   // m_controllers[i] es la IA de m_players[i]
    std::vector<int> m_hitFrames;
    HechizosGestor m_spells;
    CombatManager m_combat;
    int m_tick = 0;

    nlohmann::json m_replayFrames = nlohmann::json::array();   // un frame por tick, en el formato de generateCombat
};