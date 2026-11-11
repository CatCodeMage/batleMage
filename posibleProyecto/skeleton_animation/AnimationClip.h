#pragma once

#include "AssimpModelLoader.h"

#include <memory>

namespace bm::anim {

class AnimationClip {
public:
    AnimationClip(std::shared_ptr<const ImportedModel> model, std::size_t clipIndex);

    const ImportedModel& model() const { return *m_model; }
    const std::string& name() const { return data().name; }
    double ticksPerSecond() const;
    double durationSeconds() const;
    // Segundos normalizados a [0, duracion) en bucle y [0, duracion] sin bucle.
    double playbackTime(double seconds, bool looping) const;
    const ImportedChannel* channel(std::size_t nodeIndex) const;
    glm::mat4 sampleLocal(std::size_t nodeIndex, double seconds, bool looping = true) const;
    void samplePose(double seconds, bool looping, std::vector<glm::mat4>& locals) const;

private:
    const ImportedClip& data() const { return m_model->clips[m_clipIndex]; }
    glm::mat4 sampleLocalTicks(std::size_t nodeIndex, double ticks) const;

    std::shared_ptr<const ImportedModel> m_model;
    std::size_t m_clipIndex;
    // Indices, no punteros: las copias del clip siguen siendo seguras.
    std::vector<std::size_t> m_channelByNode;
};

} // namespace bm::anim
