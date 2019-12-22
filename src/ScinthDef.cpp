#include "ScinthDef.hpp"

#include "VGenInstance.hpp"

namespace scin {

ScinthDef::ScinthDef(const std::vector<VGenInstance>& instances): m_instances(instances) {}

ScinthDef::~ScinthDef() {}

// TODO: so here's a question champ: where do the abstract, easily testable objects that are just doing string
// manipulation and the Vulkan codes collide? Cuz right now you can't write unit tests on objects that touch Vulkan,
// at least until the SwiftShader bubble pops. Is now that time?

bool ScinthDef::buildShaders(bool keepSources) {
    // Vertex shader is constant for the moment as we focus on fragment shaders.
    return false;
}

} // namespace scin
