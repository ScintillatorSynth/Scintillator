#include "core/AbstractScinthDef.hpp"

#include "core/VGen.hpp"

namespace scin {

AbstractScinthDef::AbstractScinthDef(const std::vector<VGen>& instances): m_instances(instances) {}

AbstractScinthDef::~AbstractScinthDef() {}

bool AbstractScinthDef::buildShaders(bool keepSources) {
    // Vertex shader is constant for the moment as we focus on fragment shaders.
    return true;
}

} // namespace scin
