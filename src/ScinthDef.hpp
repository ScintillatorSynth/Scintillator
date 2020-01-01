#ifndef SRC_SCINTHDEF_HPP_
#define SRC_SCINTHDEF_HPP_

#include <memory>

namespace scin {

namespace vk {
class CommandPool;
class Device;
class Pipeline;
class Shader;
class ShaderCompiler;
class Uniform;
}

class AbstractScinthDef;
class Scinth;

/*! A ScinthDef encapsulates all of the graphics state that can be reused across individual instances of Scinths.
 */
class ScinthDef {
public:
    ScinthDef(std::shared_ptr<vk::Device> device, std::shared_ptr<const AbstractScinthDef> abstractScinthDef);
    ~ScinthDef();

    /*! Given the AbstractScinthDef, build the Vulkan objects that can be re-used across all running Scinth instances
     * of this ScinthDef.
     */
    bool build(std::shared_ptr<vk::ShaderCompiler> compiler);

    /*! Create a running instance Scinth of this ScinthDef.
     */
    std::unique_ptr<Scinth> play(std::shared_ptr<vk::CommandPool> commandPool);

private:
    std::shared_ptr<vk::Device> m_device;
    std::shared_ptr<const AbstractScinthDef> m_abstractScinthDef;
    std::unique_ptr<vk::Shader> m_vertexShader;
    std::unique_ptr<vk::Shader> m_fragmentShader;
    std::unique_ptr<vk::Uniform> m_uniform;
    std::unique_ptr<vk::Pipeline> m_pipeline;
};

} // namespace scin

#endif // SRC_SCINTHDEF_HPP_
