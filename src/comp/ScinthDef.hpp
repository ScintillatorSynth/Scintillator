#ifndef SRC_COMP_SCINTHDEF_HPP_
#define SRC_COMP_SCINTHDEF_HPP_

#include "vulkan/Vulkan.hpp"

#include <memory>
#include <vector>

namespace scin {

namespace base {
class AbstractScinthDef;
}

namespace vk {
class CommandPool;
class Device;
class HostBuffer;
class Sampler;
class Shader;
}

namespace comp {

class Canvas;
class Pipeline;
class SamplerFactory;
class Scinth;
class ShaderCompiler;

/*! A ScinthDef encapsulates all of the graphics state that can be reused across individual instances of Scinths.
 */
class ScinthDef {
public:
    ScinthDef(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas,
              std::shared_ptr<vk::CommandPool> commandPool, std::shared_ptr<SamplerFactory> samplerFactory,
              std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef);
    ~ScinthDef();

    /*! Given the AbstractScinthDef, build the Vulkan objects that can be re-used across all running Scinth instances
     * of this ScinthDef.
     *
     * \param compiler A pointer to the ShaderCompiler, used to compile the shaders generated by the AbstractScinthDef.
     * \return true on success, false on error.
     */
    bool build(ShaderCompiler* compiler);

    std::shared_ptr<Canvas> canvas() const { return m_canvas; }
    std::shared_ptr<vk::CommandPool> commandPool() const { return m_commandPool; }
    std::shared_ptr<const base::AbstractScinthDef> abstract() const { return m_abstract; }
    std::shared_ptr<vk::HostBuffer> vertexBuffer() const { return m_vertexBuffer; }
    std::shared_ptr<vk::HostBuffer> indexBuffer() const { return m_indexBuffer; }
    VkDescriptorSetLayout layout() const { return m_descriptorSetLayout; }
    const std::vector<std::shared_ptr<vk::Sampler>>& fixedSamplers() const { return m_fixedSamplers; }
    const std::vector<std::shared_ptr<vk::Sampler>>& parameterizedSamplers() const { return m_parameterizedSamplers; }
    std::shared_ptr<vk::Sampler> emptySampler() const { return m_emptySampler; }
    std::shared_ptr<Pipeline> drawPipeline() const { return m_drawPipeline; }
    std::shared_ptr<Pipeline> computePipeline() const { return m_computePipeline; }

private:
    bool buildVertexData();
    bool buildDescriptorLayout();
    bool buildSamplers();

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<Canvas> m_canvas;
    std::shared_ptr<vk::CommandPool> m_commandPool;
    std::shared_ptr<SamplerFactory> m_samplerFactory;
    std::shared_ptr<const base::AbstractScinthDef> m_abstract;
    std::shared_ptr<vk::HostBuffer> m_vertexBuffer;
    std::shared_ptr<vk::HostBuffer> m_indexBuffer;
    std::shared_ptr<vk::Shader> m_computeShader;
    std::shared_ptr<vk::Shader> m_vertexShader;
    std::shared_ptr<vk::Shader> m_fragmentShader;
    VkDescriptorSetLayout m_descriptorSetLayout;
    std::vector<std::shared_ptr<vk::Sampler>> m_fixedSamplers;
    std::vector<std::shared_ptr<vk::Sampler>> m_parameterizedSamplers;
    std::shared_ptr<vk::Sampler> m_emptySampler;
    std::shared_ptr<Pipeline> m_drawPipeline;
    std::shared_ptr<Pipeline> m_computePipeline;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_SCINTHDEF_HPP_
