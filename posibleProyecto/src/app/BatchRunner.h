#pragma once
#include "app/Options.h"

namespace app {

// Modo de analisis SIN ventana ni OpenGL: simula muchas batallas a toda velocidad y resume los resultados.
// Demuestra que la simulacion esta desacoplada del render (base para entrenar ML mas adelante).
int runBatchSimulation(const AppOptions& options);

}  // namespace app
