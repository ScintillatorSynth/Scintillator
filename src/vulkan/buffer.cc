#include "vulkan/buffer.h"

#include "vulkan/device.h"

namespace scin {

namespace vk {

Buffer::Buffer(std::shared_ptr<Device> device) :
    device_(device) {
}

}    // namespace scin

}    // namespace vk

