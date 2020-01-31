#ifndef SRC_OSC_ADDRESS_HPP_
#define SRC_OSC_ADDRESS_HPP_

#include "lo/lo.h"

namespace scin { namespace osc {

/*! Makes a copy of a supplied lo_address object and allows for smart pointer wrapping for automatic deletion.
 */
class Address {
public:
    Address(lo_address address);
    ~Address();

    lo_address get() { return m_address; }

private:
    lo_address m_address;
};

} // namespace osc
} // namespace scin

#endif // SRC_OSC_ADDRESS_HPP_
