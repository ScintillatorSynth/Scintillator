#ifndef SRC_COMP_FRAME_CONTEXT_HPP_
#define SRC_COMP_FRAME_CONTEXT_HPP_

#include <memory>
#include <vector>

namespace scin {

namespace vk {
class CommandBuffer;
class Image;
}

namespace comp {

class Node;

/*! Encapsulates all of the Vulkan resources needed to capture a single frame of rendering. Populated by the Node tree,
 * and then retained by the requesting rendering object until render is complete. This helps to ensure that even if the
 * Scinths issuing the render commands are deleted the underlying resources will not be freed until the render has
 * completed.
 */
class FrameContext {
public:
    /*!
     * \param imageIndex The index of the imageView in the Canvas we will be rendering in to. Renders are often
     *        pipelined, requiring copies of some modifiable resources to be made for each possible simultaneous render.
     *        The maximum depth of the pipeline is the number of output framebuffers that are configured. The imageIndex
     *        indicates which of the framebuffers to render this frame to, and so which corresponding set of resources
     *        to use.
     */
    FrameContext(size_t imageIndex);
    ~FrameContext() = default;

    /*! Resets the FrameContext object to be reused in a new frame render. Clears all internal references to old
     * objects, meaning that some Vulkan or other resources might be deleted.
     *
     * \param frameTime The point in time at which to build this frame for, measured in seconds from the first frame
     *        rendered.
     */
    void reset(double frameTime);

    /*! Append a Node to the list of nodes. Keeping a reference to every Node used in the rendering of the frame ensures
     * that nodes (and their associated Vulkan resources) won't be deleted until every pipelined render that they have
     * submitted commands to is done.
     */
    void appendNode(std::shared_ptr<Node> node) { m_nodes.emplace_back(node); }

    /*! Append a secondary compute command buffer to the list of compute commands.
     *
     * \param compute The secondary compute command buffer to append.
     */
    void appendComputeCommands(std::shared_ptr<vk::CommandBuffer> compute) { m_computeCommands.emplace_back(compute); }

    /*! Append a secondary draw command buffer to the list of draw commands.
     *
     * \param draw The secondary draw command buffer to append.
     */
    void appendDrawCommands(std::shared_ptr<vk::CommandBuffer> draw) { m_drawCommands.emplace_back(draw); }

    /*! Append an image object to the list of images, for retention during the lifetime of the frame.
     *
     * \param image The image to retain.
     */
    void appendImage(std::shared_ptr<vk::Image> image) { m_images.emplace_back(image); }

    void setComputePrimary(std::shared_ptr<vk::CommandBuffer> compute) { m_computePrimary = compute; }
    void setDrawPrimary(std::shared_ptr<vk::CommandBuffer> draw) { m_drawPrimary = draw; }

    size_t imageIndex() const { return m_imageIndex; }
    double frameTime() const { return m_frameTime; }
    const std::vector<std::shared_ptr<vk::CommandBuffer>>& computeCommands() const { return m_computeCommands; }
    const std::vector<std::shared_ptr<vk::CommandBuffer>>& drawCommands() const { return m_drawCommands; }
    std::shared_ptr<vk::CommandBuffer> computePrimary() { return m_computePrimary; }
    std::shared_ptr<vk::CommandBuffer> drawPrimary() { return m_drawPrimary; }

private:
    size_t m_imageIndex;
    double m_frameTime;
    std::vector<std::shared_ptr<Node>> m_nodes;
    std::vector<std::shared_ptr<vk::CommandBuffer>> m_computeCommands;
    std::vector<std::shared_ptr<vk::CommandBuffer>> m_drawCommands;
    std::vector<std::shared_ptr<vk::Image>> m_images;
    std::shared_ptr<vk::CommandBuffer> m_computePrimary;
    std::shared_ptr<vk::CommandBuffer> m_drawPrimary;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_FRAME_CONTEXT_HPP_

