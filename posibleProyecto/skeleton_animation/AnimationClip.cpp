#include "AnimationClip.h"

// GLM exige activar esta extension para descomponer las referencias TRS.
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace bm::anim {
namespace {

constexpr std::size_t NO_CHANNEL = (std::numeric_limits<std::size_t>::max)();
// El mismo fallback que AssimpModelLoader para archivos sin frecuencia declarada.
constexpr double DEFAULT_TICKS_PER_SECOND = 25.0;

bool finiteValue(const glm::vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

bool finiteValue(const glm::quat& value) {
    const float lengthSquared = glm::dot(value, value);
    return std::isfinite(lengthSquared) && lengthSquared > 0.0f;
}

template<class Key>
void validateKeys(const std::vector<Key>& keys) {
    double previous = -(std::numeric_limits<double>::infinity)();
    for (const Key& key : keys) {
        if (!std::isfinite(key.timeTicks) || key.timeTicks < previous || !finiteValue(key.value)) {
            throw std::invalid_argument("AnimationClip: keyframes invalidos o desordenados");
        }
        previous = key.timeTicks;
    }
}

glm::vec3 interpolate(const glm::vec3& a, const glm::vec3& b, float factor) {
    return glm::mix(a, b, factor);
}

glm::quat interpolate(const glm::quat& a, const glm::quat& b, float factor) {
    return glm::normalize(glm::slerp(glm::normalize(a), glm::normalize(b), factor));
}

template<class Key, class Value>
Value sampleKeys(const std::vector<Key>& keys, double ticks, const Value& fallback) {
    if (keys.empty()) return fallback;
    // upper_bound tambien resuelve tiempos duplicados: gana la ultima clave.
    const auto right = std::upper_bound(keys.begin(), keys.end(), ticks,
        [](double time, const Key& key) { return time < key.timeTicks; });
    if (right == keys.begin()) return keys.front().value;
    if (right == keys.end()) return keys.back().value;
    const Key& left = *(right - 1);
    const float factor = static_cast<float>((ticks - left.timeTicks) /
                                            (right->timeTicks - left.timeTicks));
    return interpolate(left.value, right->value, factor);
}

} // namespace

AnimationClip::AnimationClip(std::shared_ptr<const ImportedModel> model, std::size_t clipIndex)
    : m_model(std::move(model)), m_clipIndex(clipIndex) {
    if (!m_model || clipIndex >= m_model->clips.size()) {
        throw std::invalid_argument("AnimationClip: modelo o indice de clip invalido");
    }
    if (!std::isfinite(data().durationTicks) || data().durationTicks < 0.0 ||
        !std::isfinite(data().ticksPerSecond) || data().ticksPerSecond < 0.0 ||
        !std::isfinite(durationSeconds())) {
        throw std::invalid_argument("AnimationClip: duracion o frecuencia invalida");
    }
    m_channelByNode.assign(m_model->nodes.size(), NO_CHANNEL);
    for (std::size_t i = 0; i < data().channels.size(); ++i) {
        const ImportedChannel& current = data().channels[i];
        if (current.nodeIndex >= m_channelByNode.size() ||
            m_channelByNode[current.nodeIndex] != NO_CHANNEL) {
            throw std::invalid_argument("AnimationClip: nodo de canal invalido o duplicado");
        }
        validateKeys(current.positions);
        validateKeys(current.rotations);
        validateKeys(current.scales);
        m_channelByNode[current.nodeIndex] = i;
    }
}

double AnimationClip::ticksPerSecond() const {
    return data().ticksPerSecond > 0.0 ? data().ticksPerSecond : DEFAULT_TICKS_PER_SECOND;
}

double AnimationClip::durationSeconds() const {
    return data().durationTicks / ticksPerSecond();
}

double AnimationClip::playbackTime(double seconds, bool looping) const {
    if (!std::isfinite(seconds)) throw std::invalid_argument("AnimationClip: tiempo no finito");
    const double duration = durationSeconds();
    if (duration == 0.0) return 0.0;
    if (!looping) return std::clamp(seconds, 0.0, duration);
    double time = std::fmod(seconds, duration);
    if (time < 0.0) time += duration;
    return time;
}

const ImportedChannel* AnimationClip::channel(std::size_t nodeIndex) const {
    const std::size_t index = m_channelByNode.at(nodeIndex);
    return index == NO_CHANNEL ? nullptr : &data().channels[index];
}

glm::mat4 AnimationClip::sampleLocal(std::size_t nodeIndex, double seconds, bool looping) const {
    return sampleLocalTicks(nodeIndex, playbackTime(seconds, looping) * ticksPerSecond());
}

void AnimationClip::samplePose(double seconds, bool looping, std::vector<glm::mat4>& locals) const {
    const double ticks = playbackTime(seconds, looping) * ticksPerSecond();
    locals.resize(m_model->nodes.size());
    for (std::size_t i = 0; i < locals.size(); ++i) locals[i] = sampleLocalTicks(i, ticks);
}

glm::mat4 AnimationClip::sampleLocalTicks(std::size_t nodeIndex, double ticks) const {
    const glm::mat4& bind = m_model->nodes.at(nodeIndex).bindLocal;
    const ImportedChannel* current = channel(nodeIndex);
    if (!current || (current->positions.empty() && current->rotations.empty() && current->scales.empty())) {
        return bind; // Exacta, incluso cuando contiene shear o una escala nula.
    }
    if (current->rotations.empty() && current->scales.empty()) {
        glm::mat4 result = bind;
        result[3] = glm::vec4(sampleKeys(current->positions, ticks, glm::vec3(bind[3])), bind[3].w);
        return result;
    }

    glm::vec3 position(bind[3]), scale(1.0f), skew(0.0f);
    glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec4 perspective(0.0f, 0.0f, 0.0f, 1.0f);
    const bool partial = current->rotations.empty() || current->scales.empty();
    if (partial && !glm::decompose(bind, scale, rotation, position, skew, perspective)) {
        throw std::invalid_argument("AnimationClip: referencia singular en un canal TRS parcial");
    }
    position = sampleKeys(current->positions, ticks, position);
    rotation = glm::normalize(sampleKeys(current->rotations, ticks, rotation));
    scale = sampleKeys(current->scales, ticks, scale);
    // Conserva tambien el shear/perspectiva de una referencia descompuesta.
    if (partial) return glm::recompose(scale, rotation, position, skew, perspective);
    return glm::scale(glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(rotation), scale);
}

} // namespace bm::anim
