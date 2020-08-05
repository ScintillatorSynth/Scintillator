#include "comp/Group.hpp"

#include "vulkan/Device.hpp"

namespace scin { namespace comp {

Group::Group(std::shared_ptr<vk::Device> device, int nodeID): Node(device, nodeID) {}

} // namespace comp
} // namespace scin

