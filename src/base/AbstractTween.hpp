#ifndef SRC_BASE_ABSTRACT_TWEEN_HPP_
#define SRC_BASE_ABSTRACT_TWEEN_HPP_

#include <glm/glm.hpp>

#include <vector>

namespace scin { namespace base {

/*! Represents the specification for a Tween curve as part of a ScinthDef specification.
 */
class AbstractTween {
public:
    enum Curve {
        kBackIn = 0,
        kBackInOut = 1,
        kBackOut = 2,
        kBounceIn = 3,
        kBounceInOut = 4,
        kBounceOut = 5,
        kCircularIn = 6,
        kCircularInOut = 7,
        kCircularOut = 8,
        kCubicIn = 9,
        kCubicInOut = 10,
        kCubicOut = 11,
        kElasticIn = 12,
        kElasticInOut = 13,
        kElasticOut = 14,
        kExponentialIn = 15,
        kExponentialInOut = 16,
        kExponentialOut = 17,
        kLinear = 18,
        kQuadraticIn = 19,
        kQuadraticInOut = 20,
        kQuadraticOut = 21,
        kQuarticIn = 22,
        kQuarticInOut = 23,
        kQuarticOut = 24,
        kQuinticIn = 25,
        kQuinticInOut = 26,
        kQuinticOut = 27,
        kSinusoidalIn = 28,
        kSinusoidalInOut = 29,
        kSinusoidalOut = 30
    };

    AbstractTween(int dimension, float sampleRate, float totalTime, bool loop, const std::vector<glm::vec4>& levels,
                  const std::vector<float>& durations, const std::vector<Curve>& curves);
    ~AbstractTween() = default;

    bool validate() const;

    int dimension() const { return m_dimension; }
    float sampleRate() const { return m_sampleRate; }
    float totalTime() const { return m_totalTime; }
    bool loop() const { return m_loop; }

    const std::vector<glm::vec4>& levels() const { return m_levels; }
    const std::vector<float>& durations() const { return m_durations; }
    const std::vector<Curve>& curves() const { return m_curves; }

private:
    int m_dimension;
    float m_sampleRate;
    float m_totalTime;
    bool m_loop;

    std::vector<glm::vec4> m_levels;
    std::vector<float> m_durations;
    std::vector<Curve> m_curves;
};

} // namespace base
} // namespace scin

#endif // SRC_BASE_TWEEN_HPP_
