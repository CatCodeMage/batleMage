#include "sim/Match.h"

#include "sim/RuleBasedAgent.h"

namespace sim {

namespace {
// La semilla de los agentes se deriva de la de la batalla para que todo sea reproducible.
std::uint64_t agentSeed(std::uint64_t battleSeed) {
    return battleSeed * 0x9E3779B97F4A7C15ull + 0x5851F42D4C957F2Dull;
}
}  // namespace

Match::Match(const BattleConfig& cfg, std::unique_ptr<IAgent> a0, std::unique_ptr<IAgent> a1)
    : battle_(cfg), agents_{std::move(a0), std::move(a1)}, agentRng_(agentSeed(cfg.seed)) {}

void Match::reset(std::uint64_t seed) {
    battle_.reset(seed);
    agentRng_ = Rng(agentSeed(seed));
    for (auto& a : agents_) a->reset();
}

void Match::tick() {
    if (battle_.outcome() != Outcome::Running) return;

    Action chosen[2] = {Action::Idle, Action::Idle};
    for (int i = 0; i < 2; ++i) {
        chosen[i] = agents_[i]->decide(battle_.observe(i), battle_.legalActions(i), agentRng_);
    }
    battle_.step(chosen[0], chosen[1]);
}

Match makeDefaultMatch(const BattleConfig& cfg) {
    AgentProfile aggressor;
    aggressor.name = "Agresivo";
    aggressor.aggression = 0.9f;
    aggressor.reactionSkill = 0.55f;
    aggressor.blockPreference = 0.3f;

    AgentProfile defender;
    defender.name = "Defensivo";
    defender.aggression = 0.6f;
    defender.reactionSkill = 0.85f;
    defender.blockPreference = 0.6f;

    return Match(cfg, std::make_unique<RuleBasedAgent>(aggressor), std::make_unique<RuleBasedAgent>(defender));
}

}  // namespace sim
