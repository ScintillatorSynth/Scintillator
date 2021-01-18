#include "base/Tween.hpp"

namespace scin { namespace base {

Tween::Tween(int dimension, float sampleRate, const std::vector<glm::vec4>& levels, const std::vector<float>& durations,
            const std::vector<Curve>& curves):
            m_dimension(dimension),
            m_sampleRate(sampleRate),
            m_levels(levels),
            m_durations(durations),
            m_curves(curves) {}


} // namespace base
} // namespace scin
