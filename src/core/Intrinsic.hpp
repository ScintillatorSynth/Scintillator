#ifndef SRC_CORE_INTRINSIC_HPP_
#define SRC_CORE_INTRINSIC_HPP_

#include <string>

namespace scin { namespace core {

enum Intrinsic : int { kNormPos, kPi, kSampler, kTime, kTexPos, kNotFound };

/*! Returns Intrinsic enum associated with name, or kNotFound if no Intrinsic found by that name.
 *
 * \param name The string name of the intrinsic, without the @ symbol at the start.
 * \return The Intrinsic enum associated with name, or kNotFound if not found.
 */
Intrinsic getIntrinsicNamed(const std::string& name);

} // namespace core

} // namespace scin

#endif // SRC_CORE_INTRINSIC_HPP_
