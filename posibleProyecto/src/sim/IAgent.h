#pragma once
#include "sim/Action.h"
#include "sim/Battle.h"
#include "sim/Rng.h"

namespace sim {

// El "cerebro" de un combatiente. Recibe lo que ve y las acciones legales, y devuelve una accion.
//
// Esta interfaz es EL punto de enchufe para el ML: hoy la implementa un bot de reglas
// (RuleBasedAgent); mas adelante una red neuronal entrenada implementara la misma interfaz
// y el resto del programa (simulacion, render, grabacion) no cambiara ni una linea.
class IAgent {
public:
    virtual ~IAgent() = default;

    virtual void reset() {}
    virtual Action decide(const Observation& obs, const ActionMask& legal, Rng& rng) = 0;
    virtual const char* name() const = 0;
};

}  // namespace sim
