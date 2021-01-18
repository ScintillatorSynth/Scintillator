#ifndef SRC_BASE_TWEEN_HPP_
#define SRC_BASE_TWEEN_HPP_

#include <glm/glm.hpp>

#include <vector>

namespace scin { namespace base {

/*! Represents the specification for a Tween curve as part of a ScinthDef specification.
 */
struct Tween {
public:
    Tween(int dimension, float sampleRate);
    ~Tween();

    enum Curve {
        kBackIn,
        kBackInOut,
        kBounceIn,
        kBounceInOut,
        kBounceOut,
        kCircularIn,
        kCircularInOut,
        kCircularOut,
        kCubicIn,
        kCubicInOut,
        kCubicOut,
        kElasticIn,
        kElasticInOut,
        kElasticOut,
        kExponentialIn,
        kExponentialInOut,
        kExponentialOut,
        kLinear,
        kQuadraticIn,
        kQuadraticInOut,
        kQuadraticOut,
        kQuarticIn,
        kQuarticInOut,
        kQuarticOut,
        kQuinticIn,
        kQuinticInOut,
        kQuinticOut,
        kSinusodalIn,
        kSinusodalInOut,
        kSinusodalOut
    };

    int dimension() const { return m_dimension; }
    float sampleRate() const { return m_sampleRate; }

    const std::vector<glm::vec4>& levels() const { return m_levels; }
    const std::vector<float>& durations() const { return m_durations; }
    const std::vector<Curve>& curves() const { return m_curve; }

private:
    int m_dimension;
    float m_sampleRate;

    std::vector<glm::vec4> m_levels;
    std::vector<float> m_durations;
    std::vector<Curve> m_curves;
};

} // namespace base
} // namespace scin

#define SRC_BASE_TWEEN_HPP_
