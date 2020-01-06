#ifndef SRC_VULKAN_COMMAND_BUFFER_HPP_
#define SRC_VULKAN_COMMAND_BUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class CommandPool;
class Device;
class Uniform;

/*! Simple wrapper class around an array of Vulkan CommandBuffer objects.
 */
class CommandBuffer {
public:
    CommandBuffer(const std::shared_ptr<Device> device, CommandPool* commandPool);
    ~CommandBuffer();

    bool create(size_t count, bool isPrimary);
    void destroy();

    /*! Associates a uniform buffer with this command buffer, allowing the destruction of the Uniform to be tied to
     * after the destruction of this CommandBuffer.
     *
     * \param uniform A shared pointer to the Uniform object, to keep the ref count above zero until after this buffer
     *        destructs.
     */
    void associateUniform(std::shared_ptr<Uniform> uniform);

    VkCommandBuffer buffer(size_t i) { return m_commandBuffers[i]; }
    size_t count() const { return m_commandBuffers.size(); }

private:
    std::shared_ptr<Device> m_device;
    CommandPool* m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::shared_ptr<Uniform> m_uniform;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_COMMAND_BUFFER_HPP_
