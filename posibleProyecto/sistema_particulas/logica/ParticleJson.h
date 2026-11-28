#pragma once

#include "../ParticleEffect.h"
#include "../ParticleFrame.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <vector>

namespace bm::particles::particleJson {

nlohmann::json escribirEstilos(const ParticleStyleCatalog& estilos);
ParticleStyleCatalog leerEstilos(const nlohmann::json& datos);

nlohmann::json escribirFrames(const std::vector<ParticleFrame>& frames);
std::vector<ParticleFrame> leerFrames(const nlohmann::json& datos, const ParticleStyleCatalog& estilos,
                                     std::size_t totalFrames);

}
