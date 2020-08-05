#ifndef SRC_COMP_ROOT_NODE_HPP_
#define SRC_COMP_ROOT_NODE_HPP_

#include "comp/Node.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace scin {

namespace audio {
class Ingress;
}

namespace base {
class AbstractScinthDef;
}

namespace vk {
class CommandPool;
class Device;
class HostBuffer;
}

namespace comp {

class Canvas;
class FrameContext;
class ImageMap;
class SamplerFactory;
class ScinthDef;
class ShaderCompiler;
class StageManager;

/*! Root object of the render tree. Maintains global objects for the render tree such as currently defined ScinthDefs
 * and image buffers. Creates the primary command buffers and render passes. Renders to a Canvas.
 */
class RootNode : public Node {
public:
    /*! Construct a root node. Root nodes assume a hard-coded nodeID of 0.
     *
     * \param device The Vulkan device.
     * \param canvas The canvas that root node will render all contained nodes to for its output render pass.
     */
    RootNode(std::shared_ptr<vk::Device> device, std::shared_ptr<Canvas> canvas);
    virtual ~RootNode() = default;

    bool create() override;
    void destroy() override;

    /*! In this case returns true if the primary command buffer had to be rebuilt, which can be useful for tracking
     * statistics about effectiveness of caching command buffers.
     */
    bool prepareFrame(std::shared_ptr<FrameContext> context) override;

    /*! Construct a ScinthDef designed to render into this RootNode and add to the local ScinthDef map.
     *
     * \param abstractScinthDef The template to build the ScinthDef from
     * \return true on success, false on failure.
     */
    bool buildScinthDef(std::shared_ptr<const base::AbstractScinthDef> abstractScinthDef);

    /*! Remove the supplied ScinthDefs from the RootNode map.
     *
     * \param names A list of ScinthDef names to remove.
     */
    void freeScinthDefs(const std::vector<std::string>& names);

    /*! Prepare to copy the provided decoded image to a suitable GPU-local buffer.
     *
     * \param imageID The integer image identifier for this
     * \param width The width of the image in pixels.
     * \param height The height of the image in pixels.
     * \param imageBuffer The bytes of the image in RGBA format.
     * \param completion A function to call once the image has been staged.
     */
    void stageImage(int imageID, uint32_t width, uint32_t height, std::shared_ptr<vk::HostBuffer> imageBuffer,
                    std::function<void()> completion);

    /*! Returns basic information about the image associated with imageID, if it exists.
     *
     * \param imageID The image ID to query
     * \param sizeOut Will store the size in bytes of the image buffer
     * \param widthOut Will store the width of the image buffer in pixels
     * \param heightOut Will store the height of the image buffer in pixels
     * \return True if the image was found and the output values have been written, false otherwise.
     */
    bool queryImage(int imageID, size_t& sizeOut, uint32_t& widthOut, uint32_t& heightOut);

    /*! Adds an audio Ingress object for provision of audio data to the GPU.
     *
     * \param ingress The ingress object to consume audio from. RootNode will attempt to extract audio data from the
     *        source on every call to prepareFrame().
     * \param imageID The image ID to associate with this audio stream, for sampling.
     * \return true on success, false on failure.
     */
    bool addAudioIngress(std::shared_ptr<audio::Ingress> ingress, int imageID);

    std::shared_ptr<StageManager> stageManager() { return m_stageManager; }

protected:
    void rebuildCommandBuffer(std::shared_ptr<FrameContext> context);

    std::shared_ptr<Canvas> m_canvas;
    std::unique_ptr<ShaderCompiler> m_shaderCompiler;
    std::shared_ptr<vk::CommandPool> m_computeCommandPool;
    std::shared_ptr<vk::CommandPool> m_drawCommandPool;
    std::shared_ptr<StageManager> m_stageManager;
    std::shared_ptr<SamplerFactory> m_samplerFactory;
    std::shared_ptr<ImageMap> m_imageMap;
    std::atomic<bool> m_commandBufferDirty;
    std::atomic<int> m_nodeSerial;

    std::mutex m_scinthDefMutex;
    std::unordered_map<std::string, std::shared_ptr<ScinthDef>> m_scinthDefs;

    std::shared_ptr<vk::CommandBuffer> m_computePrimary;
    std::shared_ptr<vk::CommandBuffer> m_drawPrimary;

    std::mutex m_nodeMutex;
    // Flat map of every node in the tree, for (amortized) O(1) access to individual nodes by ID and maintaining
    // guarantee that each nodeID uniquely identifies a single node in the tree. The value is an iterator from the child
    // list of the parent Node containing this one.
    std::unordered_map<int, std::list<std::shared_ptr<Node>>::iterator> m_nodeMap;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_ROOT_NODE_HPP_