#include "osc/Address.hpp"

namespace scin { namespace osc {

Address::Address(lo_address address) {
    m_address = lo_address_new_with_proto(lo_address_get_protocol(address), lo_address_get_hostname(address),
                                          lo_address_get_port(address));
}

Address::~Address() { lo_address_free(m_address); }

} // namespace osc
} // namespace scin
