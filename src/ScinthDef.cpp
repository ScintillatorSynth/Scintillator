#include "ScinthDef.hpp"

#include "VGenInstance.hpp"

namespace scin {

ScinthDef::ScinthDef(const std::vector<VGenInstance>& instances): m_instances(instances) {}

ScinthDef::~ScinthDef() {}

} // namespace scin
