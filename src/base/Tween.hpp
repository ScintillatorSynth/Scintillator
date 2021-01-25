#ifndef SRC_BASE_TWEEN_HPP_
#define SRC_BASE_TWEEN_HPP_

#include <glm/glm.hpp>

#include <vector>

namespace scin { namespace base {

/*! Represents the specification for a Tween curve as part of a ScinthDef specification.
 */
struct Tween {
public:
    enum Curve {
        kBackIn = 0,
        kBackInOut = 1,
        kBounceIn = 2,
        kBounceInOut = 3,
        kBounceOut = 4,
        kCircularIn = 5,
        kCircularInOut = 6,
        kCircularOut = 7,
        kCubicIn = 8,
        kCubicInOut = 9,
        kCubicOut = 10,
        kElasticIn = 11,
        kElasticInOut = 12,
        kElasticOut = 13,
        kExponentialIn = 14,
        kExponentialInOut = 15,
        kExponentialOut = 16,
        kLinear = 17,
        kQuadraticIn = 18,
        kQuadraticInOut = 19,
        kQuadraticOut = 20,
        kQuarticIn = 21,
        kQuarticInOut = 22,
        kQuarticOut = 23,
        kQuinticIn = 24,
        kQuinticInOut = 25,
        kQuinticOut = 26,
        kSinusodalIn = 27,
        kSinusodalInOut = 28,
        kSinusodalOut = 29
    };

    Tween(int dimension, float sampleRate, float totalTime, bool loop, const std::vector<glm::vec4>& levels,
          const std::vector<float>& durations, const std::vector<Curve>& curves);
    ~Tween() = default;

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
