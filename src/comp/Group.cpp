#include "comp/Group.hpp"

#include "comp/FrameContext.hpp"
#include "vulkan/Device.hpp"

namespace scin { namespace comp {

Group::Group(std::shared_ptr<vk::Device> device, int nodeID): Node(device, nodeID) {}

bool Group::create() { return true; }

bool Group::prepareFrame(std::shared_ptr<FrameContext> context) {
    bool rebuildRequired = false;
    for (auto child : m_children) {
        context->appendNode(child);
        rebuildRequired |= child->prepareFrame(context);
    }
    return rebuildRequired;
}

void Group::setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                          const std::vector<std::pair<int, float>>& indexedValues) {
    for (auto child : m_children) {
        child->setParameters(namedValues, indexedValues);
    }
}

} // namespace comp
} // namespace scin
