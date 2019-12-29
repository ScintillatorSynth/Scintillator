#ifndef SRC_COMPOSITOR_HPP_
#define SRC_COMPOSITOR_HPP_

#include <memory>
#include <string>
#include <unordered_map>

namespace scin {

namespace vk {
    class CommandPool;
    class Device;
    class ShaderCompiler;
} // namespace vk

class AbstractScinthDef;
class ScinthDef;

/*! A Compositor keeps the ScinthDef instance dictionary as well as all running Scinths. It can render on command to a
 * supplied FrameBuffer, which is typically owned by either a Window/SwapChain combination or an Offscreen render pass
 * in the case of non-realtime rendering.
 */
class Compositor {
public:
    Compositor(std::shared_ptr<vk::Device> device);
    ~Compositor();

    bool create();

    bool buildScinthDef(std::shared_ptr<const AbstractScinthDef> abstractScinthDef);

    /*! Unload the shader compiler, releasing the resources associated with it.
     *
     * This can be used to save some memory by releasing the shader compiler, at the cost of increased latency in
     * defining new ScinthDefs, as the compiler will have to be loaded again first.
     */
    void releaseCompiler();

private:
    std::shared_ptr<vk::Device> m_device;

    std::unique_ptr<vk::ShaderCompiler> m_shaderCompiler;
    std::unique_ptr<vk::CommandPool> m_commandPool;
    std::unordered_map<std::string, std::shared_ptr<const ScinthDef>> m_scinthDefs;
};

} // namespace scin

#endif // SRC_COMPOSITOR_HPP_
