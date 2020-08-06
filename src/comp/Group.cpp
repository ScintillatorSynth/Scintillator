#include "comp/Group.hpp"

#include "vulkan/Device.hpp"

namespace scin { namespace comp {

Group::Group(std::shared_ptr<vk::Device> device, int nodeID, std::weak_ptr<Node> parent):
    Node(device, nodeID, parent) {}

bool Group::create() { return true; }

void Group::destroy() {
    for (auto child : m_children) {
        child->destroy();
    }
    m_children.clear();
}

bool Group::prepareFrame(std::shared_ptr<FrameContext> context) {
    bool rebuildRequired = false;
    for (auto child : m_children) {
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

