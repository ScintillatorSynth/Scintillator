#ifndef SRC_VULKAN_COMMAND_POOL_H_
#define SRC_VULKAN_COMMAND_POOL_H_

#include "vulkan/scin_include_vulkan.h"

#include <memory>
#include <vector>

namespace scin {

namespace vk {

class Buffer;
class Device;
class Pipeline;
class Swapchain;
class Uniform;

class CommandPool {
  public:
    CommandPool(std::shared_ptr<Device> device);
    ~CommandPool();

    bool Create();
    void Destroy();

    bool CreateCommandBuffers(Swapchain* swapchain, Pipeline* pipeline, Buffer* vertex_buffer, Uniform* uniform);

    VkCommandBuffer command_buffer(size_t i) { return command_buffers_[i]; }

  private:
    std::shared_ptr<Device> device_;
    VkCommandPool command_pool_;
    std::vector<VkCommandBuffer> command_buffers_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_COMMAND_POOL_H_

