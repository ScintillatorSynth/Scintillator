#ifndef SRC_COMP_SCINTH_HPP_
#define SRC_COMP_SCINTH_HPP_

#include "comp/Node.hpp"
#include "vulkan/Vulkan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace scin {

namespace vk {
class CommandBuffer;
class CommandPool;
class Device;
class DeviceBuffer;
class DeviceImage;
class HostBuffer;
}

namespace comp {

class FrameContext;
class ImageMap;
class ScinthDef;

/*! Represents a running, controllable instance of a ScinthDef.
 */
class Scinth : public Node {
public:
    Scinth(std::shared_ptr<vk::Device> device, int nodeID, std::shared_ptr<ScinthDef> scinthDef,
           std::shared_ptr<ImageMap> imageMap);
    ~Scinth();

    /*! Do any one-time setup on this Scinth, including creating a uniform buffer, samplers, and descriptor sets as
     * needed.
     *
     * \return true if successful, false if not.
     */
    bool create() override;

    /*! Prepare for the next frame to render. In particular updates the uniform buffer associated with the context, if
     * present, and any other operations to prepare a frame to render at the provided time.
     *
     * \params context The shared state object for the current frame render.
     * \return true if a command buffer rebuild happened, false if not.
     */
    bool prepareFrame(std::shared_ptr<FrameContext> context) override;

    /*! Set parameters on this Scinth, causing a command buffer rebuild on the next call to prepareFrame.
     *  Parameters are set in order starting with the named values and followed by the indexed values, so any duplicate
     *  settings will respect the last value written.
     *
     * \param namedValues The pairs of parameter names and values to set
     * \param indexedValues The pairs of parameter indices and values to set
     */
    void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                       const std::vector<std::pair<int, float>>& indexedValues) override;

    /*! Determines the paused or playing status of the Node. TODO: should paused nodes still render? Unlike in audio,
     * a paused VGen can still produce a still frame.
     *
     * \param run If false, will pause the Node. If true, will play it.
     */
    void setRun(bool run) override { m_running = run; }

    void forEach(std::function<void(std::shared_ptr<Node> node)>) override {}
    void appendState(std::vector<Node::NodeState>& nodes) override;

    bool isGroup() const override { return false; }
    bool isScinth() const override { return true; }

private:
    bool allocateDescriptors();
    void updateDescriptors();
    void rebuildBuffers();

    std::shared_ptr<ScinthDef> m_scinthDef;
    std::shared_ptr<ImageMap> m_imageMap;
    bool m_cueued;
    VkDescriptorPool m_descriptorPool;
    size_t m_numberOfParameters;
    bool m_commandBuffersDirty;

    bool m_running;
    double m_startTime;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<std::shared_ptr<vk::DeviceImage>> m_fixedImages;
    std::vector<std::shared_ptr<vk::DeviceImage>> m_parameterizedImages;
    std::vector<std::shared_ptr<vk::HostBuffer>> m_uniformBuffers;
    std::vector<std::shared_ptr<vk::DeviceBuffer>> m_computeBuffers;
    // The parameter index and the currently bound image Ids for parameterized images.
    std::vector<std::pair<size_t, size_t>> m_parameterizedImageIDs;
    std::shared_ptr<vk::CommandBuffer> m_computeCommands;
    std::shared_ptr<vk::CommandBuffer> m_drawCommands;
    std::unique_ptr<float[]> m_parameterValues;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_SCINTH_HPP_
