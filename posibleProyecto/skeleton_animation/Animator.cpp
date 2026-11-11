#include "Animator.h"

#include <cmath>
#include <stdexcept>
#include <utility>

namespace bm::anim {

Animator::Animator(std::shared_ptr<const Skeleton> skeleton)
    : m_skeleton(std::move(skeleton)) {
    if (!m_skeleton) throw std::invalid_argument("Animator: esqueleto nulo");
    const auto& meshes = m_skeleton->model().meshes;
    m_inverseMeshBind.resize(meshes.size(), glm::mat4(1.0f));
    m_boneMatrices.resize(meshes.size());
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        if (!meshes[i].isSkinned()) continue;
        const float determinant = glm::determinant(meshes[i].meshBindGlobal);
        if (!std::isfinite(determinant) || determinant == 0.0f) {
            throw std::invalid_argument("Animator: meshBindGlobal no invertible");
        }
        m_inverseMeshBind[i] = glm::inverse(meshes[i].meshBindGlobal);
        m_boneMatrices[i].resize(meshes[i].bones.size(), glm::mat4(1.0f));
    }
    evaluatePose();
}

void Animator::play(std::shared_ptr<const AnimationClip> clip, bool looping) {
    if (clip && &clip->model() != &m_skeleton->model()) {
        throw std::invalid_argument("Animator: el clip pertenece a otro modelo");
    }
    m_clip = std::move(clip);
    m_looping = looping;
    reset();
}

void Animator::update(double deltaSeconds) {
    if (!std::isfinite(deltaSeconds) || deltaSeconds < 0.0) {
        throw std::invalid_argument("Animator: delta de tiempo invalido");
    }
    if (m_clip && !isFinished()) setTime(m_timeSeconds + deltaSeconds);
}

void Animator::setTime(double seconds) {
    if (!std::isfinite(seconds)) throw std::invalid_argument("Animator: tiempo no finito");
    m_timeSeconds = m_clip ? m_clip->playbackTime(seconds, m_looping) : 0.0;
    evaluatePose();
}

void Animator::reset() {
    setTime(0.0);
}

bool Animator::isFinished() const {
    return m_clip && !m_looping && m_timeSeconds >= m_clip->durationSeconds();
}

const std::vector<glm::mat4>& Animator::boneMatrices(std::size_t meshIndex) const {
    return m_boneMatrices.at(meshIndex);
}

const glm::mat4& Animator::meshTransform(std::size_t meshIndex) const {
    const ImportedMesh& mesh = m_skeleton->model().meshes.at(meshIndex);
    return mesh.isSkinned() ? mesh.meshBindGlobal : m_globals.at(mesh.attachedNode);
}

void Animator::evaluatePose() {
    if (m_clip) {
        m_clip->samplePose(m_timeSeconds, m_looping, m_locals);
    } else {
        m_locals.resize(m_skeleton->nodeCount());
        for (std::size_t i = 0; i < m_locals.size(); ++i) m_locals[i] = m_skeleton->node(i).bindLocal;
    }
    m_skeleton->calculateGlobals(m_locals, m_globals);

    const auto& meshes = m_skeleton->model().meshes;
    for (std::size_t i = 0; i < meshes.size(); ++i) {
        for (std::size_t j = 0; j < meshes[i].bones.size(); ++j) {
            const ImportedBone& bone = meshes[i].bones[j];
            // mesh local -> bone -> escena animada -> mesh local de referencia.
            // La raiz importada ya esta incluida en m_globals.
            m_boneMatrices[i][j] = m_inverseMeshBind[i] * m_globals[bone.nodeIndex] * bone.offsetMatrix;
        }
    }
}

} // namespace bm::anim
