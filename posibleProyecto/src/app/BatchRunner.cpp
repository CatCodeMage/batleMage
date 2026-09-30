#include "app/BatchRunner.h"

#include <chrono>
#include <cstdio>

#include "sim/Match.h"

namespace app {

int runBatchSimulation(const AppOptions& opt) {
    const auto t0 = std::chrono::steady_clock::now();

    sim::BattleConfig cfg = toBattleConfig(opt);
    sim::Match match = sim::makeDefaultMatch(cfg);

    int wins[2] = {0, 0};
    int draws = 0;
    int timeouts = 0;
    double totalTime = 0.0;
    sim::FighterStats total[2];
    long long steps = 0;

    for (int run = 0; run < opt.simulateRuns; ++run) {
        match.reset(opt.seed + static_cast<std::uint64_t>(run));
        while (match.battle().outcome() == sim::Outcome::Running) {
            match.tick();
            ++steps;
        }
        const sim::Battle& b = match.battle();
        totalTime += b.time();
        if (b.timedOut()) ++timeouts;
        switch (b.outcome()) {
            case sim::Outcome::Fighter0Wins: ++wins[0]; break;
            case sim::Outcome::Fighter1Wins: ++wins[1]; break;
            default: ++draws; break;
        }
        for (int i = 0; i < 2; ++i) {
            total[i].casts += b.stats(i).casts;
            total[i].hits += b.stats(i).hits;
            total[i].blocked += b.stats(i).blocked;
            total[i].dodged += b.stats(i).dodged;
            total[i].damageDealt += b.stats(i).damageDealt;
        }
    }

    const double n = static_cast<double>(opt.simulateRuns);
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();

    std::printf("Simuladas %d batallas (semillas %llu..%llu, limite %.0f s)\n", opt.simulateRuns,
                static_cast<unsigned long long>(opt.seed), static_cast<unsigned long long>(opt.seed + opt.simulateRuns - 1), cfg.maxDuration);
    std::printf("  Victorias: %s %d | %s %d | empates %d | acabadas por tiempo %d\n", match.agent(0).name(), wins[0], match.agent(1).name(),
                wins[1], draws, timeouts);
    std::printf("  Duracion media: %.1f s\n", totalTime / n);
    for (int i = 0; i < 2; ++i) {
        std::printf("  %-10s por batalla: lanzamientos %.1f | impactos %.1f | bloqueos %.1f | esquivas %.1f | dano %.1f\n", match.agent(i).name(),
                    total[i].casts / n, total[i].hits / n, total[i].blocked / n, total[i].dodged / n, total[i].damageDealt / n);
    }
    std::printf("  Tiempo de calculo: %.3f s (%.0f pasos/s, %.0fx tiempo real)\n", secs, static_cast<double>(steps) / secs,
                static_cast<double>(steps) * sim::kDt / secs);
    return 0;
}

}  // namespace app
