#include "../GestorParticulasLogica.h"
#include "../ParticleJson.h"
#include "../../ParticlePlayback.h"
#include "../../ParticleSystem.h"

#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace bm::particles;
using nlohmann::json;

namespace {

void comprobar(bool correcto, const char* mensaje) {
    if (!correcto) throw std::runtime_error(mensaje);
}

template <typename Funcion>
void debeRechazar(Funcion ejecutar) {
    try {
        ejecutar();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error("Se acepto un dato invalido");
}

void probarEmisores() {
    GestorParticulasLogica gestor;
    comprobar(gestor.registrarFrame(0).emitters.empty(), "Frame inicial vacio");
    const glm::vec3 posicion(1.0f, 2.0f, 3.0f), direccion(0.0f, 1.0f, 0.0f);
    const auto fijo = gestor.crearEmisor(presets::Fire, posicion);
    const auto estela = gestor.crearEmisor(presets::MagicTrail, posicion, direccion,
        VinculoEmisor{TipoVinculo::Hechizo, 25});
    const auto humo = gestor.crearEmisor(presets::Fire, posicion, direccion,
        VinculoEmisor{TipoVinculo::Hechizo, 25});
    const auto aura = gestor.crearEmisor(presets::Smoke, posicion, direccion,
        VinculoEmisor{TipoVinculo::Personaje, 1});
    const auto cristal = gestor.crearEmisor(presets::Fire, posicion, direccion,
        VinculoEmisor{TipoVinculo::Asset, 4});

    comprobar(fijo > 0 && fijo < estela && estela < humo && humo < aura && aura < cristal, "IDs unicos");
    comprobar(!gestor.vinculoDe(fijo) && gestor.vinculoDe(humo)->propietario == 25, "Emisor fijo y multiples emisores por propietario");
    comprobar(gestor.vinculoDe(estela)->tipo == TipoVinculo::Hechizo &&
        gestor.vinculoDe(aura)->tipo == TipoVinculo::Personaje && gestor.vinculoDe(cristal)->tipo == TipoVinculo::Asset,
        "Tipos de vinculo");

    debeRechazar([&] { gestor.registrarFrame(0); });
    debeRechazar([&] { gestor.registrarFrame(2); });
    const auto primero = gestor.registrarFrame(1);
    comprobar(primero.emitters.size() == 5 && primero.emitters[0].emitterId == fijo &&
        primero.emitters[1].emitterId == estela && primero.emitters[4].emitterId == cristal, "Orden estable");
    const auto& original = primero.emitters[1];
    comprobar(original.startTick == 1 && original.position == posicion && original.direction == direccion, "Nacimiento");

    const glm::vec3 siguiente(4.0f, 5.0f, 6.0f), otraDireccion(1.0f, 0.0f, 0.0f);
    comprobar(gestor.actualizarPosicion(estela, siguiente, otraDireccion), "Actualizar posicion y direccion");
    const auto segundo = gestor.registrarFrame(2);
    const auto& actualizado = segundo.emitters[1];
    comprobar(actualizado.position == siguiente && actualizado.direction == otraDireccion, "Transformacion mundial");
    comprobar(actualizado.emitterId == original.emitterId && actualizado.styleId == original.styleId &&
        actualizado.seed == original.seed && actualizado.startTick == original.startTick, "Identidad estable");

    comprobar(gestor.detenerEmisor(estela) && !gestor.detenerEmisor(estela) &&
        !gestor.actualizarPosicion(estela, posicion, direccion) && !gestor.vinculoDe(estela), "Detener un emisor");
    const auto reemplazo = gestor.crearEmisor(presets::MagicTrail, siguiente);
    const auto tercero = gestor.registrarFrame(3);
    comprobar(tercero.emitters.size() == 5 && tercero.emitters[1].emitterId == humo &&
        tercero.emitters.back().emitterId == reemplazo && reemplazo > cristal &&
        tercero.emitters.back().startTick == 3, "El cese no afecta a otros emisores ni recicla IDs");

    gestor.generarExplosion(presets::Explosion, posicion, 8);
    gestor.reset();
    const auto reinicio = gestor.registrarFrame(0);
    comprobar(reinicio.emitters.empty() && reinicio.bursts.empty(), "Reset de emisores y bursts pendientes");
    comprobar(gestor.crearEmisor(presets::Fire, posicion) == fijo &&
        gestor.registrarFrame(1).emitters.front().seed == primero.emitters.front().seed, "Reset determinista");
}

void probarBursts() {
    const auto grabar = [] {
        GestorParticulasLogica gestor;
        const auto id = gestor.generarExplosion(presets::Explosion, glm::vec3(0.0f), 8);
        const auto siguiente = gestor.generarExplosion(presets::Explosion, glm::vec3(1.0f), 12, glm::vec3(0, 1, 0));
        debeRechazar([&] { gestor.registrarFrame(1); });
        const auto primero = gestor.registrarFrame(0);
        const auto segundo = gestor.registrarFrame(1);
        comprobar(primero.bursts.size() == 2 && primero.bursts[0].eventId == id &&
            primero.bursts[1].eventId == siguiente && siguiente > id &&
            primero.bursts[1].count == 12 && primero.bursts[1].direction == glm::vec3(0, 1, 0),
            "Bursts distintos en el mismo tick y en orden de creacion");
        comprobar(primero.bursts[0].seed != primero.bursts[1].seed &&
            segundo.bursts.empty() && segundo.emitters.empty(), "Bursts de un solo fotograma");
        return particleJson::escribirFrames({primero, segundo});
    };
    comprobar(grabar() == grabar(), "Los bursts y las semillas son reproducibles");
}

void probarJsonYReproduccion() {
    auto estilos = presets::makeCatalog();
    estilos.at(presets::Fire).particlesPerSecond = 60.0f;
    GestorParticulasLogica gestor(estilos);
    const auto emisor = gestor.crearEmisor(presets::Fire, glm::vec3(0.0f));
    gestor.generarExplosion(presets::Explosion, glm::vec3(2.0f), 3);
    std::vector<ParticleFrame> frames{gestor.registrarFrame(0)};
    gestor.actualizarPosicion(emisor, glm::vec3(3, 0, 0), glm::vec3(1, 0, 0));
    frames.push_back(gestor.registrarFrame(1));
    gestor.detenerEmisor(emisor);
    frames.push_back(gestor.registrarFrame(2));

    const json catalogo = particleJson::escribirEstilos(gestor.estilos());
    const json secuencia = particleJson::escribirFrames(frames);
    auto recuperados = particleJson::leerEstilos(json::parse(catalogo.dump()));
    auto framesRecuperados = particleJson::leerFrames(json::parse(secuencia.dump()), recuperados, 3);
    comprobar(catalogo[0].size() == 16 && particleJson::escribirEstilos(recuperados) == catalogo, "Los 16 campos del estilo");
    comprobar(secuencia[0]["emitters"][0].size() == 6 && secuencia[0]["bursts"][0].size() == 6 &&
        particleJson::escribirFrames(framesRecuperados) == secuencia, "Campos de emisores, bursts y frames");
    comprobar(recuperados.at(presets::Fire).particlesPerSecond == 60.0f &&
        recuperados.at(presets::Explosion).emission == ParticleEmission::Radial &&
        framesRecuperados[1].emitters.front().position == glm::vec3(3, 0, 0), "Valores del replay recuperado");

    ParticleSystem sistema;
    ParticlePlayback reproduccion(sistema);
    sistema.setStyles(std::move(recuperados));
    reproduccion.setFrames(std::move(framesRecuperados), 60);
    reproduccion.seek(0);
    comprobar(sistema.particles().size() == 4, "Burst y emisor llegan a ParticleSystem");
    reproduccion.seek(2);
    comprobar(sistema.particles().size() == 5, "El emisor parado deja sus particulas vivas");
    const auto posicion = sistema.particles().back().position;
    reproduccion.seek(0);
    reproduccion.seek(2);
    comprobar(sistema.particles().size() == 5 && sistema.particles().back().position == posicion, "Retroceso determinista");
}

void probarValidacion() {
    GestorParticulasLogica gestor;
    const glm::vec3 origen(0.0f);
    gestor.crearEmisor(presets::Fire, origen);
    gestor.generarExplosion(presets::Explosion, origen, 10);
    const auto primero = gestor.registrarFrame(0);
    const auto segundo = gestor.registrarFrame(1);
    const auto estilos = presets::makeCatalog();
    const json catalogo = particleJson::escribirEstilos(estilos);
    const json frames = particleJson::escribirFrames({primero, segundo, {}, segundo});

    debeRechazar([&] { gestor.crearEmisor(987, origen); });
    debeRechazar([&] { gestor.generarExplosion(presets::Explosion, origen, 0); });
    debeRechazar([&] { gestor.crearEmisor(presets::Fire, origen, origen, VinculoEmisor{TipoVinculo::Asset, 0}); });
    debeRechazar([&] { gestor.crearEmisor(presets::Fire, glm::vec3((std::numeric_limits<float>::infinity)())); });
    debeRechazar([&] { particleJson::leerFrames(frames, estilos, 5); });
    debeRechazar([&] { particleJson::leerFrames(frames, estilos, 4); }); // Reutiliza el emisor tras un hueco.

    const auto rechazarEstilo = [&](auto editar) {
        auto invalido = catalogo;
        editar(invalido);
        debeRechazar([&] { particleJson::leerEstilos(invalido); });
    };
    rechazarEstilo([](json& valor) { valor.push_back(valor[0]); });
    rechazarEstilo([](json& valor) { valor[0]["minLifeTicks"] = 0; });
    rechazarEstilo([](json& valor) { valor[0]["blend"] = "otro"; });
    rechazarEstilo([](json& valor) { valor[0]["startColor"][3] = 2.0; });
    rechazarEstilo([](json& valor) { valor[0]["styleId"] = std::uint64_t{1} << 32; });

    const auto rechazarFrame = [&](auto editar) {
        auto invalido = particleJson::escribirFrames({primero, segundo});
        editar(invalido);
        debeRechazar([&] { particleJson::leerFrames(invalido, estilos, 2); });
    };
    rechazarFrame([](json& valor) { valor[0]["emitters"][0]["position"] = {1, 2}; });
    rechazarFrame([](json& valor) { valor[0]["emitters"][0]["direction"][0] = 1e50; });
    rechazarFrame([](json& valor) { valor[0]["emitters"][0]["emitterId"] = -1; });
    rechazarFrame([](json& valor) { valor[0]["emitters"][0]["startTick"] = 1; });
    rechazarFrame([](json& valor) { valor[1]["emitters"][0]["seed"] = 0; });
    rechazarFrame([](json& valor) { valor[0]["emitters"].push_back(valor[0]["emitters"][0]); });
    rechazarFrame([](json& valor) { valor[1]["bursts"] = valor[0]["bursts"]; });
    rechazarFrame([](json& valor) { valor[0]["bursts"][0]["count"] = 3.5; });
    rechazarFrame([](json& valor) { valor[0]["bursts"][0]["styleId"] = 987; });

    auto grande = particleJson::escribirFrames({primero});
    const auto maxId = (std::numeric_limits<std::uint64_t>::max)();
    grande[0]["emitters"][0]["emitterId"] = maxId;
    grande[0]["bursts"][0]["seed"] = maxId;
    const auto recuperado = particleJson::leerFrames(json::parse(grande.dump()), estilos, 1);
    comprobar(recuperado[0].emitters[0].emitterId == maxId &&
        recuperado[0].bursts[0].seed == maxId, "Se conservan los 64 bits de los IDs y semillas");
}

}

int main() {
    try {
        probarEmisores();
        probarBursts();
        probarJsonYReproduccion();
        probarValidacion();
        std::cout << "4 suites de logica, JSON y reproduccion de particulas superadas\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
