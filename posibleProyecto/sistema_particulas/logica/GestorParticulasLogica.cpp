#include "GestorParticulasLogica.h"

#include "ValidacionParticulas.h"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace bm::particles {
namespace {

std::uint64_t semilla(std::uint64_t id) {
    return id * 0x9e3779b97f4a7c15ULL;
}

void comprobarTransformacion(const glm::vec3& posicion, const glm::vec3& direccion) {
    if (!detalle::esFinito(posicion) || !detalle::esFinito(direccion))
        throw std::invalid_argument("Posicion o direccion de particulas invalida");
}

}

GestorParticulasLogica::GestorParticulasLogica(ParticleStyleCatalog estilos) : estilos_(std::move(estilos)) {
    for (const auto& [id, estilo] : estilos_) {
        if (!detalle::estiloValido(estilo))
            throw std::invalid_argument("Estilo de particulas invalido: " + std::to_string(id));
    }
}

void GestorParticulasLogica::comprobarEstilo(ParticleStyleId estilo) const {
    if (!estilos_.contains(estilo))
        throw std::invalid_argument("Estilo de particulas desconocido: " + std::to_string(estilo));
}

std::uint64_t GestorParticulasLogica::nuevoId() {
    if (siguienteId_ == 0) throw std::overflow_error("Se agotaron los IDs de particulas");
    return siguienteId_++;
}

EmitterId GestorParticulasLogica::crearEmisor(ParticleStyleId estilo, const glm::vec3& posicion,
                                             const glm::vec3& direccion, std::optional<VinculoEmisor> vinculo) {
    comprobarEstilo(estilo);
    comprobarTransformacion(posicion, direccion);
    if (vinculo && vinculo->propietario <= INVALID_ENTITY)
        throw std::invalid_argument("El propietario del emisor debe tener un ID valido");

    const EmitterId id = nuevoId();
    emisores_.emplace(id, EmisorActivo{{id, estilo, posicion, direccion, siguienteFrame_, semilla(id)}, vinculo});
    return id;
}

bool GestorParticulasLogica::actualizarPosicion(EmitterId emisor, const glm::vec3& posicion,
                                              const glm::vec3& direccion) {
    const auto activo = emisores_.find(emisor);
    if (activo == emisores_.end()) return false;
    comprobarTransformacion(posicion, direccion);
    activo->second.estado.position = posicion;
    activo->second.estado.direction = direccion;
    return true;
}

bool GestorParticulasLogica::detenerEmisor(EmitterId emisor) {
    return emisores_.erase(emisor) != 0;
}

std::optional<VinculoEmisor> GestorParticulasLogica::vinculoDe(EmitterId emisor) const {
    const auto activo = emisores_.find(emisor);
    return activo == emisores_.end() ? std::nullopt : activo->second.vinculo;
}

BurstId GestorParticulasLogica::generarExplosion(ParticleStyleId estilo, const glm::vec3& posicion,
                                                std::uint32_t cantidad, const glm::vec3& direccion) {
    comprobarEstilo(estilo);
    comprobarTransformacion(posicion, direccion);
    if (cantidad == 0) throw std::invalid_argument("El burst necesita una cantidad positiva");

    const BurstId id = nuevoId();
    pendientes_.push_back({id, estilo, posicion, direccion, cantidad, semilla(id)});
    return id;
}

ParticleFrame GestorParticulasLogica::registrarFrame(std::size_t frameIndex) {
    if (frameIndex != static_cast<std::size_t>(siguienteFrame_))
        throw std::invalid_argument("El fotograma de particulas debe ser el siguiente indice del replay");
    if (siguienteFrame_ == (std::numeric_limits<int>::max)())
        throw std::length_error("Demasiados fotogramas de particulas");

    ParticleFrame frame;
    frame.emitters.reserve(emisores_.size());
    for (const auto& [id, emisor] : emisores_) frame.emitters.push_back(emisor.estado);
    frame.bursts.swap(pendientes_);
    ++siguienteFrame_;
    return frame;
}

void GestorParticulasLogica::reset() {
    emisores_.clear();
    pendientes_.clear();
    siguienteId_ = 1;
    siguienteFrame_ = 0;
}

}
