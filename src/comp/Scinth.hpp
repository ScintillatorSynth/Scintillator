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

    void destroy() override;

    /*! Prepare for the next frame to render.
     *
     * Update the Uniform buffer associated with the imageIndex, if present, and any other operations to prepare a
     * frame to render at the provided time.
     *
     */
    bool prepareFrame(std::shared_ptr<FrameContext> context) override;
    void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                       const std::vector<std::pair<int, float>> indexedValues) override;

private:
    bool allocateDescriptors();
    void updateDescriptors();
    void rebuildBuffers();

    bool m_cueued;
    double m_startTime;

    // Keep a reference to the ScinthDef, so that it does not get deleted until all referring Scinths have also been
    // deleted.
    std::shared_ptr<ScinthDef> m_scinthDef;
    std::shared_ptr<ImageMap> m_imageMap;
    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    std::vector<std::shared_ptr<vk::DeviceImage>> m_fixedImages;
    std::vector<std::shared_ptr<vk::DeviceImage>> m_parameterizedImages;
    std::vector<std::unique_ptr<vk::HostBuffer>> m_uniformBuffers;
    std::vector<std::unique_ptr<vk::DeviceBuffer>> m_computeBuffers;
    // The parameter index and the currently bound image Ids for parameterized images.
    std::vector<std::pair<size_t, size_t>> m_parameterizedImageIDs;
    std::shared_ptr<vk::CommandBuffer> m_computeCommands;
    std::shared_ptr<vk::CommandBuffer> m_drawCommands;
    std::unique_ptr<float[]> m_parameterValues;
    size_t m_numberOfParameters;
    bool m_commandBuffersDirty;
};

} // namespace comp

} // namespace scin

#endif // SRC_COMP_SCINTH_HPP_
