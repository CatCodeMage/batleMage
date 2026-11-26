#include "ParticleJson.h"

#include "ValidacionParticulas.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace bm::particles::particleJson {
namespace {

using nlohmann::json;

void comprobarArray(const json& valor, const char* campo) {
    if (!valor.is_array()) throw std::invalid_argument(std::string(campo) + " debe ser un array");
}

template <typename Int>
Int leerEntero(const json& datos, const char* campo) {
    const auto& valor = datos.at(campo);
    // get<Int>() por si solo admite fracciones y conversiones con perdida de rango.
    if (valor.is_number_unsigned()) {
        const auto numero = valor.get<std::uint64_t>();
        if (std::in_range<Int>(numero)) return static_cast<Int>(numero);
    } else if (valor.is_number_integer()) {
        const auto numero = valor.get<std::int64_t>();
        if (std::in_range<Int>(numero)) return static_cast<Int>(numero);
    }
    throw std::invalid_argument(std::string(campo) + " debe ser un entero dentro de su rango");
}

float leerReal(const json& valor, const char* campo) {
    if (valor.is_number()) {
        const double numero = valor.get<double>();
        if (std::isfinite(numero) && std::abs(numero) <= (std::numeric_limits<float>::max)())
            return static_cast<float>(numero);
    }
    throw std::invalid_argument(std::string(campo) + " debe contener un numero finito representable");
}

template <glm::length_t N>
glm::vec<N, float> leerVector(const json& datos, const char* campo) {
    const auto& valor = datos.at(campo);
    comprobarArray(valor, campo);
    if (valor.size() != N)
        throw std::invalid_argument(std::string(campo) + " debe tener " + std::to_string(N) + " componentes");

    glm::vec<N, float> vector(0.0f);
    for (glm::length_t i = 0; i < N; ++i) vector[i] = leerReal(valor[i], campo);
    return vector;
}

json escribirVector(const glm::vec3& valor) { return {valor.x, valor.y, valor.z}; }
json escribirVector(const glm::vec4& valor) { return {valor.x, valor.y, valor.z, valor.w}; }

ParticleBlend leerBlend(const json& datos) {
    const auto& valor = datos.at("blend");
    if (valor == "alpha") return ParticleBlend::Alpha;
    if (valor == "additive") return ParticleBlend::Additive;
    throw std::invalid_argument("blend de particulas desconocido");
}

ParticleEmission leerEmision(const json& datos) {
    const auto& valor = datos.at("emission");
    if (valor == "directional") return ParticleEmission::Directional;
    if (valor == "radial") return ParticleEmission::Radial;
    throw std::invalid_argument("emission de particulas desconocida");
}

void comprobarEstilo(ParticleStyleId id, const ParticleStyle& estilo) {
    if (!detalle::estiloValido(estilo))
        throw std::invalid_argument("Estilo de particulas invalido: " + std::to_string(id));
}

ParticleEmitterState leerEmisor(const json& datos) {
    return {
        .emitterId = leerEntero<std::uint64_t>(datos, "emitterId"),
        .styleId = leerEntero<ParticleStyleId>(datos, "styleId"),
        .position = leerVector<3>(datos, "position"),
        .direction = leerVector<3>(datos, "direction"),
        .startTick = leerEntero<int>(datos, "startTick"),
        .seed = leerEntero<std::uint64_t>(datos, "seed")
    };
}

ParticleBurst leerBurst(const json& datos) {
    return {
        .eventId = leerEntero<std::uint64_t>(datos, "eventId"),
        .styleId = leerEntero<ParticleStyleId>(datos, "styleId"),
        .position = leerVector<3>(datos, "position"),
        .direction = leerVector<3>(datos, "direction"),
        .count = leerEntero<std::uint32_t>(datos, "count"),
        .seed = leerEntero<std::uint64_t>(datos, "seed")
    };
}

json escribirFrame(const ParticleFrame& frame) {
    json emisores = json::array();
    for (const auto& emisor : frame.emitters) {
        emisores.push_back({
            {"emitterId", emisor.emitterId}, {"styleId", emisor.styleId},
            {"position", escribirVector(emisor.position)}, {"direction", escribirVector(emisor.direction)},
            {"startTick", emisor.startTick}, {"seed", emisor.seed}
        });
    }
    json bursts = json::array();
    for (const auto& burst : frame.bursts) {
        bursts.push_back({
            {"eventId", burst.eventId}, {"styleId", burst.styleId},
            {"position", escribirVector(burst.position)}, {"direction", escribirVector(burst.direction)},
            {"count", burst.count}, {"seed", burst.seed}
        });
    }
    return {{"emitters", std::move(emisores)}, {"bursts", std::move(bursts)}};
}

struct HistorialEmisor {
    ParticleStyleId estilo;
    std::uint64_t semilla;
    int inicio;
    int ultimoFrame;
};

}

nlohmann::json escribirEstilos(const ParticleStyleCatalog& estilos) {
    std::vector<ParticleStyleId> ids;
    ids.reserve(estilos.size());
    for (const auto& [id, estilo] : estilos) ids.push_back(id);
    std::sort(ids.begin(), ids.end());

    json datos = json::array();
    for (const auto id : ids) {
        const auto& estilo = estilos.at(id);
        comprobarEstilo(id, estilo);
        datos.push_back({
            {"styleId", id}, {"material", estilo.material},
            {"blend", estilo.blend == ParticleBlend::Alpha ? "alpha" : "additive"},
            {"emission", estilo.emission == ParticleEmission::Directional ? "directional" : "radial"},
            {"particlesPerSecond", estilo.particlesPerSecond},
            {"minLifeTicks", estilo.minLifeTicks}, {"maxLifeTicks", estilo.maxLifeTicks},
            {"minSpeed", estilo.minSpeed}, {"maxSpeed", estilo.maxSpeed},
            {"direction", escribirVector(estilo.direction)}, {"coneAngleRadians", estilo.coneAngleRadians},
            {"acceleration", escribirVector(estilo.acceleration)},
            {"startSize", estilo.startSize}, {"endSize", estilo.endSize},
            {"startColor", escribirVector(estilo.startColor)}, {"endColor", escribirVector(estilo.endColor)}
        });
    }
    return datos;
}

ParticleStyleCatalog leerEstilos(const nlohmann::json& datos) {
    comprobarArray(datos, "particleStyles");
    ParticleStyleCatalog estilos;
    estilos.reserve(datos.size());
    for (const auto& valor : datos) {
        const auto id = leerEntero<ParticleStyleId>(valor, "styleId");
        ParticleStyle estilo{
            .material = valor.at("material").get<std::string>(),
            .blend = leerBlend(valor),
            .emission = leerEmision(valor),
            .particlesPerSecond = leerReal(valor.at("particlesPerSecond"), "particlesPerSecond"),
            .minLifeTicks = leerEntero<int>(valor, "minLifeTicks"),
            .maxLifeTicks = leerEntero<int>(valor, "maxLifeTicks"),
            .minSpeed = leerReal(valor.at("minSpeed"), "minSpeed"),
            .maxSpeed = leerReal(valor.at("maxSpeed"), "maxSpeed"),
            .direction = leerVector<3>(valor, "direction"),
            .coneAngleRadians = leerReal(valor.at("coneAngleRadians"), "coneAngleRadians"),
            .acceleration = leerVector<3>(valor, "acceleration"),
            .startSize = leerReal(valor.at("startSize"), "startSize"),
            .endSize = leerReal(valor.at("endSize"), "endSize"),
            .startColor = leerVector<4>(valor, "startColor"),
            .endColor = leerVector<4>(valor, "endColor")
        };
        comprobarEstilo(id, estilo);
        if (!estilos.emplace(id, std::move(estilo)).second)
            throw std::invalid_argument("Estilo de particulas duplicado: " + std::to_string(id));
    }
    return estilos;
}

nlohmann::json escribirFrames(const std::vector<ParticleFrame>& frames) {
    json datos = json::array();
    for (const auto& frame : frames) datos.push_back(escribirFrame(frame));
    return datos;
}

std::vector<ParticleFrame> leerFrames(const nlohmann::json& datos, const ParticleStyleCatalog& estilos,
                                     std::size_t totalFrames) {
    comprobarArray(datos, "particleFrames");
    if (datos.size() != totalFrames || !std::in_range<int>(totalFrames))
        throw std::invalid_argument("particleFrames debe tener un fotograma por cada entrada de frames");

    std::vector<ParticleFrame> frames;
    frames.reserve(totalFrames);
    std::unordered_map<std::uint64_t, HistorialEmisor> historial;
    std::unordered_set<std::uint64_t> eventos;

    for (const auto& valor : datos) {
        const int tick = static_cast<int>(frames.size());
        const auto& emisores = valor.at("emitters");
        const auto& bursts = valor.at("bursts");
        comprobarArray(emisores, "emitters");
        comprobarArray(bursts, "bursts");
        auto& frame = frames.emplace_back();
        frame.emitters.reserve(emisores.size());
        frame.bursts.reserve(bursts.size());

        for (const auto& elemento : emisores) {
            const auto emisor = leerEmisor(elemento);
            if (emisor.emitterId == 0 || !estilos.contains(emisor.styleId) || emisor.startTick < 0 || emisor.startTick > tick)
                throw std::invalid_argument("ID, estilo o startTick de emisor invalido en particleFrames[" + std::to_string(tick) + "]");

            const auto [entrada, nuevo] = historial.try_emplace(emisor.emitterId,
                HistorialEmisor{emisor.styleId, emisor.seed, emisor.startTick, tick});
            auto& previo = entrada->second;
            if (!nuevo && (previo.ultimoFrame != tick - 1 || previo.estilo != emisor.styleId ||
                           previo.semilla != emisor.seed || previo.inicio != emisor.startTick)) {
                throw std::invalid_argument("Emisor duplicado, reutilizado o con identidad distinta: " +
                    std::to_string(emisor.emitterId) + " en particleFrames[" + std::to_string(tick) + "]");
            }
            previo.ultimoFrame = tick;
            frame.emitters.push_back(emisor);
        }

        for (const auto& elemento : bursts) {
            const auto burst = leerBurst(elemento);
            if (burst.eventId == 0 || burst.count == 0 || !estilos.contains(burst.styleId) ||
                !eventos.insert(burst.eventId).second) {
                throw std::invalid_argument("ID, estilo o cantidad de burst invalido en particleFrames[" + std::to_string(tick) + "]");
            }
            frame.bursts.push_back(burst);
        }
    }
    return frames;
}

}
