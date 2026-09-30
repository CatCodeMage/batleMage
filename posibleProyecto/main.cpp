// Batallas magicas 3D autonomas - demo de la fase 0.
// Este archivo solo arranca: lee las opciones y elige el modo. Toda la logica vive en src/.

#include <exception>
#include <iostream>
#include <string>

#include "app/Application.h"
#include "app/BatchRunner.h"
#include "app/Options.h"

int main(int argc, char** argv) {
    app::AppOptions options;
    std::string error;
    if (!app::parseArgs(argc, argv, options, error)) {
        std::cerr << "Error: " << error << "\n\n";
        app::printUsage();
        return 1;
    }
    if (options.help) {
        app::printUsage();
        return 0;
    }

    try {
        if (options.simulateRuns > 0) return app::runBatchSimulation(options);   // sin ventana
        app::Application application(options);
        return application.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
