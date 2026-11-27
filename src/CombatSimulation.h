#pragma once
#include <filesystem>
#include <memory>
#include <vector>
#include "personaje.h"
#include "controladorMaquinaEstados.h"
#include "hechizosGestor.h"
#include <string>
#include "CombatManager.h"
#include "Collision.h"
#include "jsonManager.h"   // readSpells
#include <cmath>
#include "Paths.h"
#include "varita.h"
#include "sistema_particulas/logica/GestorParticulasLogica.h"

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
    void recordEnding();

private:
    void recordFrame(bool ending = false);
    std::string describeWinner() const;
    void createPlayers(const std::filesystem::path& combatConfigFile);
    void connectOpponents();
    size_t opponentIndexOf(size_t playerIndex) const;

    void printSpellsInFlight() const;

    void applySpellHits();
    bool tryHitAnyPlayer(Hechizo& spell);
    void recordImpactVFX(const std::string& type, const glm::vec3& position, const glm::vec3& normal, float duration = 1.9f);
    size_t countAlivePlayers() const;

    void countDownHitEffects();

    const char* replayEntityOfPlayer(size_t playerIndex) const;

    // unique_ptr: la IA guarda un puntero a su rival y el personaje uno a su IA,
    // asi que sus direcciones no pueden cambiar cuando el vector crece
    std::vector<std::unique_ptr<Personaje>> m_players;
    std::vector<std::unique_ptr<ControladorMaquinaEstados>> m_controllers;

    std::vector<int> m_hitFrames;
    struct PlayerReplayState {
        glm::vec3 position{0.0f};
        ReplayAnimationState animation;
    };
    std::vector<PlayerReplayState> m_playerReplayStates;
    GeneradorIds   m_ids;      // dueño de los ids; DEBE ir antes que m_spells
    HechizosGestor m_spells;

    CombatManager m_combat;

    // Gestion de colisiones geometricas
    CollisionManager m_collisionManager;

    int m_tick = 0;

    nlohmann::json m_replayFrames = nlohmann::json::array();   // un frame por tick, en el formato de generateCombat
    nlohmann::json m_replayImpacts = nlohmann::json::array();
    bm::particles::GestorParticulasLogica m_particulas;
    std::vector<bm::particles::ParticleFrame> m_particleFrames;

    std::vector<Varita> createWands();

};
