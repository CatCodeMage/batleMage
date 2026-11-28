#pragma once

#include "AssimpModelLoader.h"

#include <memory>
#include <span>
#include <unordered_map>

namespace bm::anim {

// Vista con propiedad compartida: el modelo debe permanecer inmutable.
// La jerarquia y los offsets siguen almacenados exclusivamente en el importador.
class Skeleton {
public:
    explicit Skeleton(std::shared_ptr<const ImportedModel> model);

    const ImportedModel& model() const { return *m_model; }
    std::size_t nodeCount() const { return m_model->nodes.size(); }
    const ImportedNode& node(std::size_t index) const;
    // -1 si no existe; los nombres duplicados son ambiguos y lanzan una excepcion.
    std::int32_t findNode(const std::string& name) const;

    // Admite usar el mismo vector como entrada y salida.
    void calculateGlobals(std::span<const glm::mat4> locals,
                          std::vector<glm::mat4>& globals) const;

private:
    std::shared_ptr<const ImportedModel> m_model;
    std::unordered_map<std::string, std::int32_t> m_nodeByName;
};

} // namespace bm::anim
