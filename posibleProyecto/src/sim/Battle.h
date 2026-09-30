#pragma once
#include <array>
#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

#include "sim/Action.h"
#include "sim/Rng.h"
#include "sim/Rules.h"

namespace sim {

// Paso de tiempo FIJO de la simulacion. No depende de los FPS de la pantalla.
constexpr float kDt = 1.0f / 60.0f;

struct Fighter {
    int id = 0;
    glm::vec3 pos{0.0f};
    glm::vec3 facing{0.0f, 0.0f, 1.0f};
    float hp = 0.0f;
    float mana = 0.0f;

    Action current = Action::Idle;   // accion en curso (Idle = libre)
    float actionElapsed = 0.0f;
    bool projectileSpawned = false;
    glm::vec3 dodgeDir{0.0f};

    float cdMagic = 0.0f;
    float cdDodge = 0.0f;
    float cdBlock = 0.0f;

    float hitTimer = 0.0f;   // >0: recien golpeado (feedback visual)
    float deadTime = 0.0f;   // tiempo desde que murio

    bool alive() const { return hp > 0.0f; }
    bool busy() const { return current != Action::Idle; }
};

struct Projectile {
    int owner = 0;
    glm::vec3 pos{0.0f};
    glm::vec3 vel{0.0f};
    float life = 0.0f;
    bool dodgeCounted = false;
};

// Contadores para el analisis posterior de la batalla.
struct FighterStats {
    int casts = 0;           // proyectiles lanzados
    int hits = 0;            // impactos conseguidos
    int blocked = 0;         // proyectiles absorbidos por su escudo
    int dodged = 0;          // proyectiles esquivados
    float damageDealt = 0.0f;
};

enum class Outcome { Running, Fighter0Wins, Fighter1Wins, Draw };

// Lo que un agente "ve" de la situacion. Es la entrada de cualquier cerebro (reglas o ML).
// Todo normalizado o en unidades del mundo; nada de punteros ni de detalles internos.
struct Observation {
    float dt = kDt;
    float selfHp = 0.0f;          // 0..1
    float selfMana = 0.0f;        // 0..1
    float opponentHp = 0.0f;      // 0..1
    float opponentMana = 0.0f;    // 0..1
    float distance = 0.0f;        // al rival
    float cdMagic = 0.0f;         // segundos que faltan
    float cdDodge = 0.0f;
    float cdBlock = 0.0f;
    bool busy = false;            // esta ejecutando una accion
    bool incoming = false;        // hay un proyectil enemigo que le apunta
    float incomingTime = 0.0f;    // segundos hasta el impacto
    float incomingDistance = 0.0f;
    float timeLeft = 0.0f;        // segundos hasta el limite de la batalla
};

struct BattleConfig {
    std::uint64_t seed = 1;
    float maxDuration = 30.0f;                 // duracion maxima de la batalla (s)
    ActionMask allowed = ActionMask::all();    // acciones permitidas en esta batalla
    Rules rules;
};

// La simulacion del combate 1 contra 1.
//  - NO usa OpenGL ni ventana: puede ejecutarse a toda velocidad sin dibujar nada.
//  - Determinista: el mismo BattleConfig y las mismas acciones dan siempre el mismo resultado.
//  - No conoce a los agentes: solo recibe la accion pedida por cada combatiente en cada paso.
//    Si la accion no es legal (sin mana, en cooldown, prohibida...), se ignora y se hace Idle.
class Battle {
public:
    explicit Battle(const BattleConfig& cfg);

    void reset(std::uint64_t seed);
    void step(Action a0, Action a1);   // avanza kDt

    Observation observe(int i) const;
    ActionMask legalActions(int i) const;

    // Lectura de estado (para el render y el analisis)
    const Fighter& fighter(int i) const { return fighters_[i]; }
    const std::vector<Projectile>& projectiles() const { return projectiles_; }
    float time() const { return time_; }
    Outcome outcome() const { return outcome_; }
    bool timedOut() const { return timedOut_; }
    const FighterStats& stats(int i) const { return stats_[i]; }
    const BattleConfig& config() const { return cfg_; }

    bool isDodging(int i) const;
    bool isShielding(int i) const;
    float actionDuration(Action a) const;
    float actionProgress(int i) const;   // 0..1 dentro de la accion en curso
    glm::vec3 shieldCenter(int i) const;

    // La configuracion de acciones permitidas se puede cambiar en caliente.
    void setAllowed(ActionMask m) { cfg_.allowed = m; }

private:
    void startAction(int i, Action a);
    void updateFighter(int i);
    void updateProjectiles();
    void evaluateOutcome();
    void spawnProjectile(const Fighter& f);
    glm::vec3 chooseDodgeDir(const Fighter& f);
    const Projectile* nearestIncoming(int i, float* outTime) const;

    BattleConfig cfg_;
    Rng rng_;
    std::array<Fighter, 2> fighters_;
    std::array<FighterStats, 2> stats_;
    std::vector<Projectile> projectiles_;
    float time_ = 0.0f;
    Outcome outcome_ = Outcome::Running;
    bool timedOut_ = false;
};

}  // namespace sim
