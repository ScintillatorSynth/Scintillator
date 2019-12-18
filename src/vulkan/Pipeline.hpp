#ifndef SRC_VULKAN_PIPELINE_HPP_
#define SRC_VULKAN_PIPELINE_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin { namespace vk {

class Device;
class Shader;
class Swapchain;
class Uniform;

class Pipeline {
public:
    Pipeline(std::shared_ptr<Device> device);
    ~Pipeline();

    enum VertexType { kFloat, kVec2, kVec3, kVec4, kIVec2, kUVec4, kDouble };

    // Call these methods before calling Create().
    void SetVertexStride(size_t size) { vertex_stride_ = size; }
    void AddVertexAttribute(VertexType type, size_t offset) {
        vertex_attributes_.push_back(std::make_pair(type, offset));
    }

    bool Create(Shader* vertex_shader, Shader* fragment_shader, Swapchain* swapchain, Uniform* uniform);
    void Destroy();

    VkRenderPass render_pass() { return render_pass_; }
    VkPipeline get() { return pipeline_; }
    VkPipelineLayout layout() { return pipeline_layout_; }

private:
    bool CreateRenderPass(Swapchain* swapchain);
    void DestroyRenderPass();

    bool CreatePipelineLayout(Uniform* uniform);
    void DestroyPipelineLayout();

    std::shared_ptr<Device> device_;

    size_t vertex_stride_;
    std::vector<std::pair<VertexType, size_t>> vertex_attributes_;

    VkRenderPass render_pass_;
    VkPipelineLayout pipeline_layout_;
    VkPipeline pipeline_;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_PIPELINE_HPP_
