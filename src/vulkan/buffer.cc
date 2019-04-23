#include "vulkan/buffer.h"

#include "vulkan/device.h"

namespace scin {

namespace vk {

Buffer::Buffer(Kind kind, std::shared_ptr<Device> device) :
    kind_(kind_),
    device_(device) {
}

}    // namespace scin

}    // namespace vk

