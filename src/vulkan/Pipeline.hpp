#ifndef SRC_VULKAN_PIPELINE_HPP_
#define SRC_VULKAN_PIPELINE_HPP_

#include "core/Manifest.hpp"
#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {

class Shape;

namespace vk {

class Canvas;
class Device;
class Shader;
class UniformLayout;

class Pipeline {
public:
    Pipeline(std::shared_ptr<Device> device);
    ~Pipeline();

    bool create(const Manifest& vertexManifest, const Shape* shape, Canvas* canvas, Shader* vertexShader,
                Shader* fragmentShader, UniformLayout* uniformLayout);
    void destroy();

    VkPipeline get() { return m_pipeline; }
    VkPipelineLayout layout() { return m_pipelineLayout; }

private:
    std::shared_ptr<Device> m_device;

    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_pipeline;
};

} // namespace vk

} // namespace scin

#endif // SRC_VULKAN_PIPELINE_HPP_
