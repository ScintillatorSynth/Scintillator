#include "base/Parameter.hpp"

namespace scin { namespace base {

Parameter::Parameter(std::string name, float defaultValue): m_name(name), m_defaultValue(defaultValue) {}

Parameter::~Parameter() {}

} // namespace base
} // namespace scin
