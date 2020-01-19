#ifndef SRC_VULKAN_RENDER_SYNC_HPP_
#define SRC_VULKAN_RENDER_SYNC_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class Device;
class Swapchain;

/*! Maintains a set of synchronization primities used for tracking state of rendering to one or more active images.
 * Useful for both windowed and offscreen rendering.
 */
class RenderSync {
public:
    RenderSync(std::shared_ptr<Device> device);
    ~RenderSync();

    /*! Create a set of synchronization primitives suitable for coordinating rendering across a maximum number of
     * simultaneously in-progress frames.
     *
     * \param inflightFrames How many sets of sync primitives to make. Should be > 0.
     * \param makeSemaphores If the semaphores (mostly useful for Swapchains) should be made.
     * \return true on success, false on failure.
     */
    bool create(size_t inFlightFrames, bool makeSemaphores);

    /*! Release synchronization primitives.
     */
    void destroy();

    void waitForFrame(size_t index);

    // Requires that the semaphores were made.
    uint32_t acquireNextImage(size_t index, Swapchain* swapchain);

    void resetFrame(size_t index);

    VkSemaphore imageAvailable(size_t index) { return m_imageAvailable[index]; }
    VkSemaphore renderFinished(size_t index) { return m_renderFinished[index]; }
    VkFence frameRendering(size_t index) { return m_frameRendering[index]; }

private:
    std::shared_ptr<Device> m_device;

    std::vector<VkSemaphore> m_imageAvailable;
    std::vector<VkSemaphore> m_renderFinished;
    std::vector<VkFence> m_frameRendering;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_RENDER_SYNC_HPP_
