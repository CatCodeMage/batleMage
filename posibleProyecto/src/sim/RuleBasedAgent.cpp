#include "sim/RuleBasedAgent.h"

#include <algorithm>

namespace sim {

void RuleBasedAgent::reset() {
    thinkTimer_ = 0.0f;
    wasIncoming_ = false;
    willReact_ = false;
    preferBlock_ = false;
}

Action RuleBasedAgent::decide(const Observation& obs, const ActionMask& legal, Rng& rng) {
    // Cuando aparece un proyectil nuevo, se decide UNA vez si se da cuenta y como reaccionara.
    if (obs.incoming && !wasIncoming_) {
        willReact_ = rng.chance(profile_.reactionSkill);
        preferBlock_ = rng.chance(profile_.blockPreference);
    }
    wasIncoming_ = obs.incoming;
    thinkTimer_ = std::max(0.0f, thinkTimer_ - obs.dt);

    if (!legal.any()) return Action::Idle;   // ocupado, sin mana, en cooldown...

    // 1. Defensa: reacciona cuando el impacto es inminente (la esquiva dura 0.45 s).
    if (obs.incoming && willReact_ && obs.incomingTime < 0.40f && obs.incomingTime > 0.05f) {
        const Action first = preferBlock_ ? Action::Block : Action::Dodge;
        const Action second = preferBlock_ ? Action::Dodge : Action::Block;
        if (legal.has(first)) return first;
        if (legal.has(second)) return second;
    }

    // 2. Ataque: guarda algo de mana para poder defenderse.
    if (thinkTimer_ <= 0.0f && legal.has(Action::CastMagic) && obs.selfMana > 0.3f) {
        thinkTimer_ = rng.range(0.10f, 0.45f);
        if (rng.chance(profile_.aggression)) return Action::CastMagic;
    }

    return Action::Idle;
}

}  // namespace sim
