#pragma once
#include <array>
#include <cstdint>
#include <memory>

#include "sim/Battle.h"
#include "sim/IAgent.h"

namespace sim {

// Una partida: une la batalla con los dos agentes que la juegan.
// Es el unico sitio donde se conectan "cerebros" y "mundo", para que Battle no conozca a los agentes.
class Match {
public:
    Match(const BattleConfig& cfg, std::unique_ptr<IAgent> a0, std::unique_ptr<IAgent> a1);

    void reset(std::uint64_t seed);
    void tick();   // un paso de simulacion: los agentes deciden y la batalla avanza

    Battle& battle() { return battle_; }
    const Battle& battle() const { return battle_; }
    const IAgent& agent(int i) const { return *agents_[i]; }

private:
    Battle battle_;
    std::array<std::unique_ptr<IAgent>, 2> agents_;
    Rng agentRng_;
};

// Partida de ejemplo: un agresivo contra un defensivo.
Match makeDefaultMatch(const BattleConfig& cfg);

}  // namespace sim
