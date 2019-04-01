#ifndef SRC_VULKAN_COMMAND_POOL_H_
#define SRC_VULKAN_COMMAND_POOL_H_

#include <memory>

namespace scin {

namespace vk {

class Device;
class Pipeline;
class Swapchain;

class CommandPool {
  public:
    CommandPool(std::shared_ptr<Device> device);
    ~CommandPool();

    bool Create();
    void Destroy();

    bool CreateCommandBuffers(Swapchain* swapchain, Pipeline* pipeline);

  private:
    std::shared_ptr<Device> device_;
    VkCommandPool command_pool_;
    std::vector<VkCommandBuffer> command_buffers_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_COMMAND_POOL_H_

