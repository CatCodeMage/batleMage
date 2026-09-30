#include "app/Options.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace app {

namespace {

bool needValue(int i, int argc, const char* flag, std::string& error) {
    if (i + 1 >= argc) {
        error = std::string("Falta el valor de ") + flag;
        return false;
    }
    return true;
}

}  // namespace

bool parseArgs(int argc, char** argv, AppOptions& o, std::string& error) {
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (!std::strcmp(a, "--help") || !std::strcmp(a, "-h")) {
            o.help = true;
        } else if (!std::strcmp(a, "--record")) {
            o.record = true;
        } else if (!std::strcmp(a, "--simulate")) {
            if (!needValue(i, argc, a, error)) return false;
            o.simulateRuns = std::atoi(argv[++i]);
        } else if (!std::strcmp(a, "--seed")) {
            if (!needValue(i, argc, a, error)) return false;
            o.seed = std::strtoull(argv[++i], nullptr, 10);
        } else if (!std::strcmp(a, "--duration")) {
            if (!needValue(i, argc, a, error)) return false;
            o.duration = static_cast<float>(std::atof(argv[++i]));
        } else if (!std::strcmp(a, "--width")) {
            if (!needValue(i, argc, a, error)) return false;
            o.width = std::atoi(argv[++i]);
        } else if (!std::strcmp(a, "--height")) {
            if (!needValue(i, argc, a, error)) return false;
            o.height = std::atoi(argv[++i]);
        } else if (!std::strcmp(a, "--fps")) {
            if (!needValue(i, argc, a, error)) return false;
            o.recordFps = std::atoi(argv[++i]);
        } else if (!std::strcmp(a, "--out")) {
            if (!needValue(i, argc, a, error)) return false;
            o.outDir = argv[++i];
        } else if (!std::strcmp(a, "--no-magic")) {
            o.allowed.set(sim::Action::CastMagic, false);
        } else if (!std::strcmp(a, "--no-dodge")) {
            o.allowed.set(sim::Action::Dodge, false);
        } else if (!std::strcmp(a, "--no-block")) {
            o.allowed.set(sim::Action::Block, false);
        } else {
            error = std::string("Opcion desconocida: ") + a;
            return false;
        }
    }
    if (o.duration <= 0.0f || o.width < 64 || o.height < 64 || o.recordFps < 1) {
        error = "Valores fuera de rango (duracion, tamano o fps)";
        return false;
    }
    return true;
}

void printUsage() {
    std::printf(
        "Batallas magicas 3D autonomas - demo\n"
        "\n"
        "Uso: posibleProyecto [opciones]\n"
        "\n"
        "  (sin opciones)     Ventana interactiva: batallas en bucle\n"
        "  --record           Graba UNA batalla como secuencia de PNG y termina\n"
        "  --simulate N       Sin ventana: simula N batallas y muestra estadisticas\n"
        "  --seed N           Semilla (misma semilla = misma batalla). Por defecto 1\n"
        "  --duration S       Duracion maxima de la batalla en segundos. Por defecto 30\n"
        "  --no-magic         Prohibe lanzar magia\n"
        "  --no-dodge         Prohibe esquivar\n"
        "  --no-block         Prohibe bloquear\n"
        "  --out DIR          Carpeta de grabacion (por defecto 'recordings')\n"
        "  --width W --height H   Resolucion (por defecto 1280x720)\n"
        "  --fps N            Fotogramas por segundo de la grabacion (por defecto 30)\n"
        "\n"
        "Teclas en la ventana:\n"
        "  ESC salir | ESPACIO pausa | R nueva batalla | F avance rapido x4\n"
        "  1 magia on/off | 2 esquivar on/off | 3 bloquear on/off\n");
}

sim::BattleConfig toBattleConfig(const AppOptions& o) {
    sim::BattleConfig c;
    c.seed = o.seed;
    c.maxDuration = o.duration;
    c.allowed = o.allowed;
    return c;
}

}  // namespace app
