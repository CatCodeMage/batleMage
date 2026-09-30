#pragma once
#include "sim/IAgent.h"

namespace sim {

// Personalidad del bot: los mismos reglas con parametros distintos dan estilos de lucha distintos.
struct AgentProfile {
    const char* name = "Bot";
    float aggression = 0.8f;       // probabilidad de atacar cuando puede
    float reactionSkill = 0.7f;    // probabilidad de reaccionar a un proyectil que llega
    float blockPreference = 0.4f;  // al reaccionar: bloquear (vs esquivar)
};

// Bot de reglas simples (maquina de estados con azar):
//   1. Si le llega un proyectil y "se da cuenta": esquiva o bloquea (si puede).
//   2. Si no, y tiene mana de sobra: lanza magia de vez en cuando.
//   3. Si no, espera.
class RuleBasedAgent : public IAgent {
public:
    explicit RuleBasedAgent(const AgentProfile& profile) : profile_(profile) {}

    void reset() override;
    Action decide(const Observation& obs, const ActionMask& legal, Rng& rng) override;
    const char* name() const override { return profile_.name; }

private:
    AgentProfile profile_;
    float thinkTimer_ = 0.0f;
    bool wasIncoming_ = false;
    bool willReact_ = false;
    bool preferBlock_ = false;
};

}  // namespace sim
