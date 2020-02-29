#ifndef SRC_COMP_SCINTHDEF_HPP_
#define SRC_COMP_SCINTHDEF_HPP_

#include <memory>

namespace scin {

namespace base {
class AbstractScinthDef;
}

namespace vk {
class CommandPool;
class Device;
class HostBuffer;
class Shader;
class UniformLayout;
}

namespace comp {

class Canvas;
class Pipeline;
class Scinth;
class ShaderCompiler;

/*! A ScinthDef encapsulates all of the graphics state that can be reused across individual instances of Scinths.
 */
class ScinthDef {
public:
    ScinthDef(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas,
              std::shared_ptr<vk::CommandPool> commandPool,
              std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef);
    ~ScinthDef();

    /*! Given the AbstractScinthDef, build the Vulkan objects that can be re-used across all running Scinth instances
     * of this ScinthDef.
     *
     * \param A pointer to the ShaderCompiler.
     * \return true on success, false on error.
     */
    bool build(ShaderCompiler* compiler);

    std::shared_ptr<Canvas> canvas() const { return m_canvas; }
    std::shared_ptr<vk::CommandPool> commandPool() const { return m_commandPool; }
    std::shared_ptr<const base::AbstractScinthDef> abstract() const { return m_abstractScinthDef; }
    std::shared_ptr<vk::HostBuffer> vertexBuffer() const { return m_vertexBuffer; }
    std::shared_ptr<vk::HostBuffer> indexBuffer() const { return m_indexBuffer; }
    std::shared_ptr<vk::UniformLayout> uniformLayout() const { return m_uniformLayout; }
    std::shared_ptr<Pipeline> pipeline() const { return m_pipeline; }

private:
    bool buildVertexData();

    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<Canvas> m_canvas;
    std::shared_ptr<vk::CommandPool> m_commandPool;
    std::shared_ptr<const base::AbstractScinthDef> m_abstractScinthDef;
    std::shared_ptr<vk::HostBuffer> m_vertexBuffer;
    std::shared_ptr<vk::HostBuffer> m_indexBuffer;
    std::shared_ptr<vk::Shader> m_vertexShader;
    std::shared_ptr<vk::Shader> m_fragmentShader;
    std::shared_ptr<vk::UniformLayout> m_uniformLayout;
    std::shared_ptr<Pipeline> m_pipeline;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_SCINTHDEF_HPP_
