#pragma once
#include <cstdint>
#include <memory>

#include "app/Options.h"
#include "app/Window.h"
#include "gfx/AutoCamera.h"
#include "gfx/Renderer.h"
#include "sim/Match.h"

struct GLFWwindow;

namespace app {

// Une las tres capas: simulacion (sim), dibujo (gfx) y ventana. Contiene los dos modos de ejecucion:
//  - interactivo: batallas en bucle a tiempo real
//  - grabacion:   una batalla a fotograma fijo, guardando cada imagen
class Application {
public:
    explicit Application(const AppOptions& options);
    int run();

private:
    int runInteractive();
    int runRecording();

    void stepOnce();                         // un paso de simulacion + camara
    void renderFrame();
    void restart(std::uint64_t seed);
    void reportOutcome();
    void updateTitle();
    void onKey(int key);

    static void keyCallback(GLFWwindow* w, int key, int scancode, int action, int mods);

    AppOptions opts_;
    std::unique_ptr<Window> window_;
    std::unique_ptr<gfx::Renderer> renderer_;
    sim::Match match_;
    gfx::AutoCamera camera_;

    std::uint64_t seed_;
    bool paused_ = false;
    float timeScale_ = 1.0f;
    bool outcomeReported_ = false;
};

}  // namespace app
