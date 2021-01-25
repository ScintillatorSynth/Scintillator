#include "base/Tween.hpp"

namespace scin { namespace base {

Tween::Tween(int dimension, float sampleRate, float totalTime, bool loop, const std::vector<glm::vec4>& levels,
             const std::vector<float>& durations, const std::vector<Curve>& curves):
    m_dimension(dimension),
    m_sampleRate(sampleRate),
    m_totalTime(totalTime),
    m_loop(loop),
    m_levels(levels),
    m_durations(durations),
    m_curves(curves) {}

bool Tween::validate() const {
    bool valid = m_dimension > 0 && m_dimension < 5 && m_dimension != 3;
    valid &= m_levels.size() > 1;
    valid &= m_levels.size() - 1 == m_durations.size();
    valid &= ((m_curves.size() == 1) || (m_curves.size() == m_durations.size()));
    return valid;
}

} // namespace base
} // namespace scin
