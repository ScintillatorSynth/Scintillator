#include "comp/Group.hpp"

namespace scin { namespace comp {

Group::Group(std::shared_ptr<Device> device, int nodeID): Node(device, nodeID) {}

} // namespace comp
} // namespace scin

