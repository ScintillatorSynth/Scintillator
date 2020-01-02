#ifndef SRC_VULKAN_COMMAND_POOL_HPP_
#define SRC_VULKAN_COMMAND_POOL_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class CommandBuffer;
class Device;

class CommandPool {
public:
    CommandPool(std::shared_ptr<Device> device);
    ~CommandPool();

    bool create();

    /*! Destroy the CommandPool and all associated CommandBuffers.
     *
     *  \note This will invalidate all CommandBuffer objects that this CommandPool created.
     */
    void destroy();

    /*! Allocate and return a set of CommandBuffer objects for use in recording GPU commands.
     *
     * \param count The number of buffers to allocate.
     * \return The CommandBuffer object, or nullptr on error.
     */
    std::shared_ptr<CommandBuffer> createBuffers(size_t count);

    VkCommandPool get() { return m_commandPool; }

private:
    std::shared_ptr<Device> m_device;
    VkCommandPool m_commandPool;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_COMMAND_POOL_HPP_
