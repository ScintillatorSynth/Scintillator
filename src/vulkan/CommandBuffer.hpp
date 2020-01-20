#ifndef SRC_VULKAN_COMMAND_BUFFER_HPP_
#define SRC_VULKAN_COMMAND_BUFFER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class Buffer;
class CommandPool;
class Device;
class Pipeline;
class Uniform;

/*! Simple wrapper class around an array of Vulkan CommandBuffer objects.
 */
class CommandBuffer {
public:
    CommandBuffer(const std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool);
    ~CommandBuffer();

    /*! Allocate the command buffers internally.
     *
     * \param count The number of buffers to allocate.
     * \param isPrimary if true will allocate primary command buffers, if false will allocate secondary.
     * \return true on success, false on error.
     */
    bool create(size_t count, bool isPrimary);

    void destroy();

    /*! Associates graphical resources with this command buffer, allowing the destruction these resources to be tied to
     * after the destruction of this CommandBuffer, ensuring the dependencies are destructed in the right order.
     *
     * \param vertexBuffer The vertex buffer bound by this CommandBuffer.
     * \param indexBuffer The index buffer bound by this CommandBuffer.
     * \param uniform The uniform, if any, bound by this CommanBuffer.
     * \param pipeline The pipeline bound by this CommandBuffer.
     */
    void associateResources(std::shared_ptr<Buffer> vertexBuffer, std::shared_ptr<Buffer> indexBuffer,
                            std::shared_ptr<Uniform> uniform, std::shared_ptr<Pipeline> pipeline);

    VkCommandBuffer buffer(size_t i) { return m_commandBuffers[i]; }
    size_t count() const { return m_commandBuffers.size(); }

private:
    std::shared_ptr<Device> m_device;
    std::shared_ptr<CommandPool> m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::shared_ptr<Buffer> m_vertexBuffer;
    std::shared_ptr<Buffer> m_indexBuffer;
    std::shared_ptr<Uniform> m_uniform;
    std::shared_ptr<Pipeline> m_pipeline;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_COMMAND_BUFFER_HPP_
