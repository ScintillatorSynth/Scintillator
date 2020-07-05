#ifndef SRC_COMP_PIPELINE_HPP_
#define SRC_COMP_PIPELINE_HPP_

#include "base/Manifest.hpp"
#include "base/RenderOptions.hpp"
#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {

namespace base {
class Shape;
}

namespace vk {
class Device;
class Shader;
}

namespace comp {

class Canvas;

class Pipeline {
public:
    Pipeline(std::shared_ptr<vk::Device> device);
    ~Pipeline();

    bool create(const base::Manifest& vertexManifest, const base::Shape* shape, Canvas* canvas,
                std::shared_ptr<vk::Shader> vertexShader, std::shared_ptr<vk::Shader> fragmentShader,
                VkDescriptorSetLayout descriptorSetLayout, size_t pushConstantBlockSize,
                const base::RenderOptions& renderOptions);
    void destroy();

    VkPipeline get() { return m_pipeline; }
    VkPipelineLayout layout() { return m_pipelineLayout; }

private:
    std::shared_ptr<vk::Device> m_device;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;

    std::shared_ptr<vk::Shader> m_vertexShader;
    std::shared_ptr<vk::Shader> m_fragmentShader;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_PIPELINE_HPP_
