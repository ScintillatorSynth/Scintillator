#ifndef SRC_COMP_STAGE_MANAGER_HPP_
#define SRC_COMP_STAGE_MANAGER_HPP_

#include "vulkan/Vulkan.hpp"

#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace scin {

namespace vk {
class CommandBuffer;
class CommandPool;
class Device;
class DeviceImage;
class HostBuffer;
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

    bool create();
    void destroy();

    /*! Typically not called on the main render thread, adds the necessary commands to a command buffer to copy the
     * host-accessible image bytes into optimal tiling and layout for read-only sampling.
     */
    bool stageImage(std::shared_ptr<vk::HostBuffer> hostBuffer, std::shared_ptr<vk::DeviceImage> deviceImage,
                    std::function<void()> completion);

    /*! If sharing the graphics queue, this function is called every frame requesting a single CommandBuffer intended
     * for submission with the frame's draw commands from the Compositor. The provided fence is for the callback
     * thread to wait, although it typically won't be signaled until all command buffers in the submit have been
     * completed, which may include a frame of rendering.
     */
    std::shared_ptr<vk::CommandBuffer> getTransferCommands(VkFence renderFence);

private:
    void callbackThreadMain();

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<vk::CommandPool> m_commandPool;

    std::atomic<bool> m_hasCommands;
    std::mutex m_commandMutex;
    std::shared_ptr<vk::CommandBuffer> m_transferCommands;
    std::vector<std::function<void()>> m_callbacks;

    std::mutex m_waitPairsMutex;
    std::condition_variable m_waitActiveCondition;
    std::deque<std::pair<VkFence, std::vector<std::function<void()>>>> m_waitPairs;
    std::thread m_callbackThread;
    std::atomic<bool> m_quit;
};

} // namespace comp

} // namespace scin

#endif
