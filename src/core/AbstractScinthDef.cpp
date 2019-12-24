#include "core/AbstractScinthDef.hpp"

#include "core/VGen.hpp"

namespace scin {

AbstractScinthDef::AbstractScinthDef(const std::vector<VGen>& instances): m_instances(instances) {}

AbstractScinthDef::~AbstractScinthDef() {}

bool AbstractScinthDef::build() { return true; }

} // namespace scin
