#include "comp/Group.hpp"

#include "comp/FrameContext.hpp"
#include "vulkan/Device.hpp"

namespace scin { namespace comp {

Group::Group(std::shared_ptr<vk::Device> device, int nodeID): Node(device, nodeID) {}

bool Group::create() { return true; }

bool Group::prepareFrame(std::shared_ptr<FrameContext> context) {
    bool rebuildRequired = false;
    for (auto node : m_subNodes) {
        context->appendNode(node);
        rebuildRequired |= node->prepareFrame(context);
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

void Group::forEach(std::function<void(std::shared_ptr<Node> node)> f) {
    for (auto node : m_subNodes) {
        node->forEach(f);
        f(node);
    }
}

void Group::appendState(std::vector<Node::NodeState>& nodes) {
    nodes.emplace_back(Node::NodeState{
        m_nodeID,
        static_cast<int>(m_subNodes.size()),
        "group",
        {}
    });
}

void Group::remove(int nodeID) {
    auto it = m_nodeMap.find(nodeID);
    m_subNodes.erase(it->second);
    m_nodeMap.erase(it);
}

void Group::insertBefore(std::shared_ptr<Node> a, int nodeB) {
    a->setParent(this);
    auto mapIt = m_nodeMap.find(nodeB);
    m_nodeMap[a->nodeID()] = m_subNodes.emplace(mapIt->second, a);
}

void Group::insertAfter(std::shared_ptr<Node> a, int nodeB) {
    a->setParent(this);
    auto mapIt = m_nodeMap.find(nodeB);
    auto listIt = mapIt->second;
    ++listIt;
    m_nodeMap[a->nodeID()] = m_subNodes.emplace(listIt, a);
}

void Group::prepend(std::shared_ptr<Node> node) {
    node->setParent(this);
    m_subNodes.push_front(node);
    m_nodeMap[node->nodeID()] = m_subNodes.begin();
}

void Group::append(std::shared_ptr<Node> node) {
    node->setParent(this);
    m_subNodes.push_back(node);
    auto it = m_subNodes.end();
    --it;
    m_nodeMap[node->nodeID()] = it;
}

void Group::replace(std::shared_ptr<Node> node, int target) {
    node->setParent(this);
    auto mapIt = m_nodeMap.find(target);
    m_nodeMap[node->nodeID()] = m_subNodes.emplace(mapIt->second, node);
    m_subNodes.erase(mapIt->second);
    if (node->nodeID() != target) {
        m_nodeMap.erase(mapIt);
    }
}

void Group::freeAll() {
    m_nodeMap.clear();
    m_subNodes.clear();
}

void Group::deepFree() {
    for (auto it = m_subNodes.begin(); it != m_subNodes.end(); /* empty increment */) {
        if ((*it)->isScinth()) {
            m_nodeMap.erase((*it)->nodeID());
            it = m_subNodes.erase(it);
        } else {
            Group* group = static_cast<Group*>((*it).get());
            group->deepFree();
            ++it;
        }
    }
}

void Group::queryTree(std::vector<Node::NodeState>& nodes) {
    for (auto node : m_subNodes) {
        node->appendState(nodes);
    }
}

} // namespace comp
} // namespace scin
