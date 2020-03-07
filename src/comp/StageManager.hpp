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

    bool create(size_t numberOfImages);
    void destroy();

    /*! Typically not called on the main render thread, adds the necessary commands to a command buffer to copy the
     * host-accessible image bytes into optimal tiling and layout for read-only sampling.
     */
    bool stageImage(std::shared_ptr<vk::HostBuffer> hostBuffer, std::shared_ptr<vk::DeviceImage> deviceImage,
                    std::function<void()> completion);


    /*! Since VkFence objects can only be waited on by one thread, we submit the transfer commands separately from the
     * compositor commands. This function must be called on the main submission thread.
     */
    bool submitTransferCommands(VkQueue queue);

private:
    void callbackThreadMain();

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<vk::CommandPool> m_commandPool;
    std::vector<VkFence> m_fences;

    struct Wait {
        Wait(): fenceIndex(0) {}
        ~Wait() = default;
        int fenceIndex;
        std::shared_ptr<vk::CommandBuffer> commands;
        std::vector<std::shared_ptr<vk::HostBuffer>> hostBuffers;
        std::vector<std::shared_ptr<vk::DeviceImage>> deviceImages;
        std::vector<std::function<void()>> callbacks;
    };

    std::atomic<bool> m_hasCommands;
    std::mutex m_commandMutex;
    Wait m_pendingWait;

    std::mutex m_waitMutex;
    std::condition_variable m_waitCondition;
    std::deque<Wait> m_waits;
    std::thread m_callbackThread;
    std::atomic<bool> m_quit;
};

} // namespace comp

} // namespace scin

#endif
