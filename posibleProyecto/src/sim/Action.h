#pragma once
#include <cstdint>

namespace sim {

// Las acciones que puede elegir un combatiente. Por ahora: magia, esquivar y bloquear con magia.
enum class Action : std::uint8_t { Idle = 0, CastMagic = 1, Dodge = 2, Block = 3 };

constexpr int kActionCount = 4;

inline const char* toString(Action a) {
    switch (a) {
        case Action::Idle:      return "Idle";
        case Action::CastMagic: return "CastMagic";
        case Action::Dodge:     return "Dodge";
        case Action::Block:     return "Block";
    }
    return "?";
}

// Conjunto de acciones (bitset). "Idle" siempre esta presente: no hacer nada nunca se puede prohibir.
// Se usa tanto para la configuracion (que acciones permite el entorno) como para
// informar a un agente de que acciones son legales en este instante (action masking).
class ActionMask {
public:
    constexpr ActionMask() : bits_(1u) {}

    static constexpr ActionMask all() {
        ActionMask m;
        m.bits_ = (1u << kActionCount) - 1u;
        return m;
    }

    constexpr bool has(Action a) const { return ((bits_ >> static_cast<unsigned>(a)) & 1u) != 0u; }

    constexpr void set(Action a, bool on) {
        if (a == Action::Idle) return;
        const unsigned bit = 1u << static_cast<unsigned>(a);
        bits_ = on ? (bits_ | bit) : (bits_ & ~bit);
    }

    constexpr void toggle(Action a) { set(a, !has(a)); }

    // Hay alguna accion ademas de Idle.
    constexpr bool any() const { return bits_ > 1u; }

private:
    unsigned bits_;
};

}  // namespace sim
