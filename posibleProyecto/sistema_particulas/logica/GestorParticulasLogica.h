#pragma once

#include "../../GeneradorIds.h"
#include "../ParticleEffect.h"
#include "../ParticleFrame.h"

#include <glm/vec3.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace bm::particles {

using EmitterId = std::uint64_t;
using BurstId = std::uint64_t;

enum class TipoVinculo { Hechizo, Personaje, Asset };

struct VinculoEmisor {
    TipoVinculo tipo;
    EntityId propietario;
};

class GestorParticulasLogica {
public:
    explicit GestorParticulasLogica(ParticleStyleCatalog estilos = presets::makeCatalog());

    // Nace en el proximo fotograma que se va a registrar. Las coordenadas son mundiales.
    EmitterId crearEmisor(ParticleStyleId estilo, const glm::vec3& posicion,
                         const glm::vec3& direccion = glm::vec3(0.0f),
                         std::optional<VinculoEmisor> vinculo = std::nullopt);
    // Para IDs inactivos o desconocidos, estas operaciones devuelven false.
    bool actualizarPosicion(EmitterId emisor, const glm::vec3& posicion, const glm::vec3& direccion);
    bool detenerEmisor(EmitterId emisor);
    std::optional<VinculoEmisor> vinculoDe(EmitterId emisor) const;

    // Se entrega una sola vez, en el proximo fotograma registrado.
    BurstId generarExplosion(ParticleStyleId estilo, const glm::vec3& posicion,
                            std::uint32_t cantidad, const glm::vec3& direccion = glm::vec3(0.0f));

    // Indice del replay: empezar en 0 y avanzar de uno en uno, incluidos los fotogramas vacios.
    ParticleFrame registrarFrame(std::size_t frameIndex);
    const ParticleStyleCatalog& estilos() const { return estilos_; }
    void reset(); // Inicia otro replay con el mismo catalogo.

private:
    struct EmisorActivo {
        ParticleEmitterState estado;
        std::optional<VinculoEmisor> vinculo;
    };

    std::uint64_t nuevoId();
    void comprobarEstilo(ParticleStyleId estilo) const;

    ParticleStyleCatalog estilos_;
    std::map<EmitterId, EmisorActivo> emisores_;
    std::vector<ParticleBurst> pendientes_;
    std::uint64_t siguienteId_ = 1;
    int siguienteFrame_ = 0;
};

}
