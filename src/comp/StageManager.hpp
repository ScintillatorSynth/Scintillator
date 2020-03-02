#ifndef SRC_COMP_STAGE_MANAGER_HPP_
#define SRC_COMP_STAGE_MANAGER_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {

namespace vk {
class CommandBuffer;
class Device;
class DeviceImage;
class HostImage;
}

namespace comp {

/*! On Discrete GPU devices at least, if not other classes of devices, copying data from host-accessible buffers to
 * GPU-only accessible memory may result in a performance improvement. As Scinths, ScinthDefs, and the general
 * Compositor may all want to do this, the StageManager centralizes management of those staging requests so they can be
 * batched and processed all at once.
 *
 * It also provides a fence for all batched staging requests, and a thread that blocks on that fence, to wait to
 * process callbacks until after content has been staged.
 *
 */
class StageManager {
public:
    StageManager(std::shared_ptr<vk::Device> device);
    ~StageManager();

    bool create(size_t inFlightFrames);

    void stageImage(std::shared_ptr<vk::HostImage> hostImage, std::shared_ptr<vk::DeviceImage> deviceImage,
                    std::function<void()> completion);

    std::shared_ptr<vk::CommandBuffer> getTransferCommands(size_t index);

private:
    std::shared_ptr<vk::Device> m_device;

    std::vector<VkSemaphore> m_transferComplete;
};

} // namespace comp

} // namespace scin

#endif
