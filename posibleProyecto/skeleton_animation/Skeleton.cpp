#include "Skeleton.h"

#include <limits>
#include <stdexcept>
#include <utility>

namespace bm::anim {

Skeleton::Skeleton(std::shared_ptr<const ImportedModel> model)
    : m_model(std::move(model)) {
    if (!m_model) throw std::invalid_argument("Skeleton: modelo nulo");
    if (nodeCount() > static_cast<std::size_t>((std::numeric_limits<std::int32_t>::max)())) {
        throw std::invalid_argument("Skeleton: demasiados nodos");
    }

    for (std::size_t i = 0; i < nodeCount(); ++i) {
        const ImportedNode& current = node(i);
        // AssimpModelLoader entrega una sola raiz y padres anteriores a sus hijos.
        if ((i == 0 && current.parentIndex != -1) ||
            (i != 0 && (current.parentIndex < 0 ||
                        static_cast<std::size_t>(current.parentIndex) >= i))) {
            throw std::invalid_argument("Skeleton: jerarquia invalida en " + current.name);
        }
        const auto [it, inserted] = m_nodeByName.emplace(current.name, static_cast<std::int32_t>(i));
        if (!inserted) it->second = -1;
    }

    for (const ImportedMesh& mesh : m_model->meshes) {
        if (mesh.attachedNode >= nodeCount()) {
            throw std::invalid_argument("Skeleton: nodo de malla fuera de rango");
        }
        for (const ImportedBone& bone : mesh.bones) {
            if (bone.nodeIndex >= nodeCount()) {
                throw std::invalid_argument("Skeleton: nodo de hueso fuera de rango");
            }
        }
    }
}

const ImportedNode& Skeleton::node(std::size_t index) const {
    return m_model->nodes.at(index);
}

std::int32_t Skeleton::findNode(const std::string& name) const {
    const auto it = m_nodeByName.find(name);
    if (it == m_nodeByName.end()) return -1;
    if (it->second < 0) throw std::invalid_argument("Skeleton: nombre ambiguo: " + name);
    return it->second;
}

void Skeleton::calculateGlobals(std::span<const glm::mat4> locals,
                                std::vector<glm::mat4>& globals) const {
    if (locals.size() != nodeCount()) {
        throw std::invalid_argument("Skeleton: numero incorrecto de matrices locales");
    }
    globals.resize(nodeCount());
    for (std::size_t i = 0; i < nodeCount(); ++i) {
        const std::int32_t parent = node(i).parentIndex;
        globals[i] = parent < 0 ? locals[i] : globals[parent] * locals[i];
    }
}

} // namespace bm::anim
