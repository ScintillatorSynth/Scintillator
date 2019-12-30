#ifndef SRC_VULKAN_COMMAND_BUFFER_HPP_
#define SRC_VULKAN_COMMAND_BUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class CommandPool;
class Device;

/*! Simple wrapper class around an array of Vulkan CommandBuffer objects.
 */
class CommandBuffer {
public:
    CommandBuffer(const std::shared_ptr<Device> device, CommandPool* commandPool);
    ~CommandBuffer();

    bool create(size_t count);
    void destroy();

    VkCommandBuffer buffer(size_t i) { return m_commandBuffers[i]; }
    size_t count() const { return m_commandBuffers.size(); }

private:
    std::shared_ptr<Device> m_device;
    CommandPool* m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_COMMAND_BUFFER_HPP_
