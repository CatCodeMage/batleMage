#pragma once

#include "AnimationClip.h"
#include "Skeleton.h"

namespace bm::anim {

// Solo CPU. Comparte datos de referencia, pero nunca el tiempo ni la pose.
class Animator {
public:
    explicit Animator(std::shared_ptr<const Skeleton> skeleton);

    // nullptr restaura la bind pose. Un nuevo play siempre empieza en cero.
    void play(std::shared_ptr<const AnimationClip> clip, bool looping = true);
    void update(double deltaSeconds);
    // Evalua inmediatamente; permite avanzar y rebobinar sin acumular deltas.
    void setTime(double seconds);
    void reset();
    double timeSeconds() const { return m_timeSeconds; }
    bool isFinished() const;

    const std::vector<glm::mat4>& nodeGlobals() const { return m_globals; }
    const std::vector<glm::mat4>& boneMatrices(std::size_t meshIndex) const;
    // Skinned: meshBindGlobal. Rigida: global animada de attachedNode.
    // La matriz de mundo a dibujar es characterModel * meshTransform(i).
    const glm::mat4& meshTransform(std::size_t meshIndex) const;

private:
    void evaluatePose();

    std::shared_ptr<const Skeleton> m_skeleton;
    std::shared_ptr<const AnimationClip> m_clip;
    double m_timeSeconds = 0.0;
    bool m_looping = true;
    std::vector<glm::mat4> m_locals, m_globals;
    std::vector<glm::mat4> m_inverseMeshBind;
    std::vector<std::vector<glm::mat4>> m_boneMatrices;
};

} // namespace bm::anim
