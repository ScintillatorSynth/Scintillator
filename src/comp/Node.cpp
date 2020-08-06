#include "comp/Node.hpp"
#include "vulkan/Device.hpp"

namespace scin { namespace comp {

Node::Node(std::shared_ptr<vk::Device> device, int nodeID):
    m_device(device), m_nodeID(nodeID), m_parent(nullptr), m_running(false) {}

} // namespace comp
} // namespace scin

