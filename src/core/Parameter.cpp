#include "core/Parameter.hpp"

namespace scin { namespace core {

Parameter::Parameter(std::string name, float defaultValue): m_name(name), m_defaultValue(defaultValue) {}

Parameter::~Parameter() {}

} // namespace core
} // namespace scin
