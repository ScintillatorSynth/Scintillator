#ifndef SRC_CORE_INTRINSIC_HPP_
#define SRC_CORE_INTRINSIC_HPP_

#include <string>

namespace scin { namespace base {

enum Intrinsic : int {
    kFragCoord,
    kNormPos,
    kPi,
    kPosition,
    kSampler,
    kTime,
    kTexPos,
    kTweenDuration,
    kTweenSampler,
    kNotFound
};

/*! Returns Intrinsic enum associated with name, or kNotFound if no Intrinsic found by that name.
 *
 * \param name The string name of the intrinsic, without the @ symbol at the start.
 * \return The Intrinsic enum associated with name, or kNotFound if not found.
 */
Intrinsic getIntrinsicNamed(const std::string& name);

} // namespace base

} // namespace scin

#endif // SRC_CORE_INTRINSIC_HPP_
