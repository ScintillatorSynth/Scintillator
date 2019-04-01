#ifndef SRC_VULKAN_PIPELINE_H_
#define SRC_VULKAN_PIPELINE_H_

#include "vulkan/scin_include_vulkan.h"

#include <memory>

namespace scin {

namespace vk {

class Device;
class Shader;
class Swapchain;

class Pipeline {
  public:
    Pipeline(std::shared_ptr<Device> device);
    ~Pipeline();

    bool Create(Shader* vertex_shader, Shader* fragment_shader,
            Swapchain* swapchain);
    void Destroy();

    VkRenderPass render_pass() { return render_pass_; }

  private:
    bool CreateRenderPass(Swapchain* swapchain);
    void DestroyRenderPass();

    bool CreatePipelineLayout();
    void DestroyPipelineLayout();

    std::shared_ptr<Device> device_;
    VkRenderPass render_pass_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
};

}    // namespace vk

}    // namespace scin

#endif    // SRC_VULKAN_PIPELINE_H_

