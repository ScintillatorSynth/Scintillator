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
    for (auto node : m_subNodes) {
        node->setParameters(namedValues, indexedValues);
    }
}

void Group::setRun(bool run) {
    for (auto node : m_subNodes) {
        node->setRun(run);
    }
}

void Group::subNodeFree(int nodeID) {
    auto it = m_nodeMap.find(nodeID);
    m_subNodes.erase(it->second);
    m_nodeMap.erase(it);
}

void Group::insertBefore(std::shared_ptr<Node> a, int nodeB) {
    auto mapIt = m_nodeMap.find(nodeB);
    m_nodeMap[a->nodeID()] = m_subNodes.emplace(mapIt->second, a);
}

void Group::insertAfter(std::shared_ptr<Node> a, int nodeB) {
    auto mapIt = m_nodeMap.find(nodeB);
    auto listIt = mapIt->second;
    ++listIt;
    m_nodeMap[a->nodeID()] = m_subNodes.emplace(listIt, a);
}

} // namespace comp
} // namespace scin
