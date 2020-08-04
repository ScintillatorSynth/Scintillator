#ifndef SRC_COMP_NODE_HPP_
#define SRC_COMP_NODE_HPP_

#include <memory>
#include <string>
#include <vector>

namespace scin {

namespace vk {
class Device;
}

namespace comp {

/*! Abstract base class for the individual elements within a rendering tree. Descendants are RootNode, Group, and
 *  Scinth.
 */
class Node {
public:
    Node(std::shared_ptr<vk::Device> device, int nodeID);
    virtual ~Node() = default;

    virtual bool create() = 0;
    virtual void destroy() = 0;

    /*! Prepare the CommandBuffers that when executed in order will render the current frame. Updates the values
     * returned by computeCommands() and drawCommands().
     *
     * \param imageIndex The index of the imageView in the Canvas we will be rendering in to.
     * \param frameTime The point in time at which to build this frame for.
     * \return A boolean indicating success in frame preparation.
     */
    virtual bool prepareFrame(size_t imageIndex, double frameTime) = 0;

    /*! Change the parameter values for this node. If this node is a group, sets the parameter values for every node in
     * this group.
     *
     * \param namedValues A vector of pairs of parameter names and new values.
     * \param indexedValues A vector of pairs of parameter indexes and new values.
     */
    virtual void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                               const std::vector<std::pair<int, float>> indexedValues) = 0;

    /*! Determines the paused or playing status of the Node. TODO: should paused nodes still render? Unlike in audio,
     * a paused VGen can still produce a still frame.
     *
     * \param run If false, will pause the Node. If true, will play it.
     */
    void setRunning(bool run) { m_running = run; }

protected:
    std::shared_ptr<vk::Device> m_device;
    int m_nodeID;
    bool m_running;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_NODE_HPP_
