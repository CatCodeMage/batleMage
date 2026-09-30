#include "app/Application.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "gfx/Recorder.h"

namespace app {

namespace {

const char* outcomeText(const sim::Battle& b, const sim::Match& m) {
    switch (b.outcome()) {
        case sim::Outcome::Fighter0Wins: return m.agent(0).name();
        case sim::Outcome::Fighter1Wins: return m.agent(1).name();
        case sim::Outcome::Draw:         return "Empate";
        case sim::Outcome::Running:      return "En curso";
    }
    return "?";
}

}  // namespace

Application::Application(const AppOptions& options)
    : opts_(options), match_(sim::makeDefaultMatch(toBattleConfig(options))), seed_(options.seed) {
    window_ = std::make_unique<Window>(opts_.width, opts_.height, "Batallas magicas", !opts_.record);
    renderer_ = std::make_unique<gfx::Renderer>();
    glfwSetWindowUserPointer(window_->handle(), this);
    glfwSetKeyCallback(window_->handle(), &Application::keyCallback);
}

void Application::keyCallback(GLFWwindow* w, int key, int, int action, int) {
    if (action != GLFW_PRESS) return;
    auto* self = static_cast<Application*>(glfwGetWindowUserPointer(w));
    if (self) self->onKey(key);
}

void Application::onKey(int key) {
    sim::Battle& b = match_.battle();
    sim::ActionMask mask = b.config().allowed;
    switch (key) {
        case GLFW_KEY_ESCAPE: window_->requestClose(); break;
        case GLFW_KEY_SPACE:  paused_ = !paused_; break;
        case GLFW_KEY_R:      restart(seed_ + 1); break;
        case GLFW_KEY_F:      timeScale_ = timeScale_ > 1.0f ? 1.0f : 4.0f; break;
        case GLFW_KEY_1:      mask.toggle(sim::Action::CastMagic); b.setAllowed(mask); break;
        case GLFW_KEY_2:      mask.toggle(sim::Action::Dodge); b.setAllowed(mask); break;
        case GLFW_KEY_3:      mask.toggle(sim::Action::Block); b.setAllowed(mask); break;
        default: break;
    }
}

void Application::stepOnce() {
    match_.tick();
    camera_.update(match_.battle(), sim::kDt);
}

void Application::renderFrame() {
    int w = 0, h = 0;
    window_->framebufferSize(w, h);
    renderer_->render(match_.battle(), camera_, w, h);
}

void Application::restart(std::uint64_t seed) {
    seed_ = seed;
    match_.reset(seed);
    camera_.reset();
    camera_.update(match_.battle(), sim::kDt);
    outcomeReported_ = false;
}

void Application::reportOutcome() {
    if (outcomeReported_) return;
    outcomeReported_ = true;
    const sim::Battle& b = match_.battle();
    std::printf("\n=== Batalla (semilla %llu) terminada en %.1f s: %s%s ===\n", static_cast<unsigned long long>(seed_),
                b.time(), outcomeText(b, match_), b.timedOut() ? " (por tiempo)" : "");
    for (int i = 0; i < 2; ++i) {
        const sim::FighterStats& s = b.stats(i);
        std::printf("  %-10s vida %5.1f | lanzamientos %2d | impactos %2d | bloqueos %2d | esquivas %2d | dano %5.1f\n",
                    match_.agent(i).name(), std::max(0.0f, b.fighter(i).hp), s.casts, s.hits, s.blocked, s.dodged, s.damageDealt);
    }
    std::fflush(stdout);
}

void Application::updateTitle() {
    const sim::Battle& b = match_.battle();
    const sim::ActionMask m = b.config().allowed;
    char buf[256];
    std::snprintf(buf, sizeof(buf), "Batallas magicas | semilla %llu | t=%4.1fs | vida %s %.0f - %.0f %s | [1]Magia:%s [2]Esquivar:%s [3]Bloquear:%s%s",
                  static_cast<unsigned long long>(seed_), b.time(), "", std::max(0.0f, b.fighter(0).hp), std::max(0.0f, b.fighter(1).hp),
                  b.outcome() == sim::Outcome::Running ? "" : "| GANA:", m.has(sim::Action::CastMagic) ? "ON" : "off",
                  m.has(sim::Action::Dodge) ? "ON" : "off", m.has(sim::Action::Block) ? "ON" : "off", paused_ ? " | PAUSA" : "");
    std::string title = buf;
    if (b.outcome() != sim::Outcome::Running) title += std::string(" ") + outcomeText(b, match_);
    window_->setTitle(title);
}

int Application::run() {
    restart(seed_);
    return opts_.record ? runRecording() : runInteractive();
}

int Application::runInteractive() {
    double prev = window_->time();
    double acc = 0.0;
    double titleTimer = 0.0;
    double endTimer = 0.0;

    while (!window_->shouldClose()) {
        window_->pollEvents();
        const double now = window_->time();
        const double frame = std::min(now - prev, 0.1);
        prev = now;

        if (!paused_) acc += frame * static_cast<double>(timeScale_);
        while (acc >= sim::kDt) {   // paso fijo: la simulacion no depende de los FPS
            stepOnce();
            acc -= sim::kDt;
        }

        renderFrame();
        window_->swapBuffers();

        if (match_.battle().outcome() != sim::Outcome::Running) {
            reportOutcome();
            endTimer += frame;
            if (endTimer > 4.0) {   // tras 4 s, empieza otra batalla automaticamente
                endTimer = 0.0;
                restart(seed_ + 1);
            }
        }

        titleTimer += frame;
        if (titleTimer > 0.2) {
            titleTimer = 0.0;
            updateTitle();
        }
    }
    return 0;
}

int Application::runRecording() {
    const int fps = opts_.recordFps;
    const int stepsPerFrame = std::max(1, static_cast<int>(std::lround(1.0 / (static_cast<double>(fps) * sim::kDt))));

    int w = 0, h = 0;
    window_->framebufferSize(w, h);
    const std::string dir = opts_.outDir + "/battle_seed" + std::to_string(seed_);
    gfx::Recorder recorder(dir, w, h);

    std::printf("Grabando a %d fps (%dx%d) en: %s\n", fps, w, h, dir.c_str());

    int tail = fps * 2;   // 2 s extra tras el final para ver el primer plano del ganador
    int guard = static_cast<int>((opts_.duration + 8.0f) * static_cast<float>(fps));

    // Fotograma inicial
    renderFrame();
    recorder.capture();
    window_->swapBuffers();

    while (!window_->shouldClose() && guard-- > 0) {
        for (int s = 0; s < stepsPerFrame; ++s) stepOnce();
        renderFrame();
        if (!recorder.capture()) {
            std::fprintf(stderr, "No se pudo guardar el fotograma %d\n", recorder.frames());
            return 1;
        }
        window_->swapBuffers();
        window_->pollEvents();

        if (match_.battle().outcome() != sim::Outcome::Running) {
            reportOutcome();
            if (--tail <= 0) break;
        }
    }

    std::printf("\nGrabados %d fotogramas (%.1f s de video a %d fps) en %s\n", recorder.frames(),
                static_cast<double>(recorder.frames()) / static_cast<double>(fps), fps, dir.c_str());
    return 0;
}

}  // namespace app
