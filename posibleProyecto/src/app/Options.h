#pragma once
#include <cstdint>
#include <string>

#include "sim/Action.h"
#include "sim/Battle.h"

namespace app {

// Todo lo que se puede configurar desde la linea de comandos.
struct AppOptions {
    bool help = false;
    bool record = false;            // modo grabacion: genera los fotogramas y termina
    int simulateRuns = 0;           // >0: modo estadisticas (sin ventana): simula N batallas
    std::uint64_t seed = 1;
    float duration = 30.0f;         // duracion maxima de la batalla (s)
    sim::ActionMask allowed = sim::ActionMask::all();
    int width = 1280;
    int height = 720;
    int recordFps = 30;
    std::string outDir = "recordings";
};

bool parseArgs(int argc, char** argv, AppOptions& out, std::string& error);
void printUsage();
sim::BattleConfig toBattleConfig(const AppOptions& o);

}  // namespace app
