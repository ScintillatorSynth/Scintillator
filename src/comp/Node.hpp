#ifndef SRC_COMP_NODE_HPP_
#define SRC_COMP_NODE_HPP_

#include <functional>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace scin {

namespace vk {
class CommandBuffer;
class Device;
}

namespace comp {

class FrameContext;
class Group;

/*! Abstract base class for the individual elements within a rendering tree. Descendants are RootNode, Group, and
 *  Scinth.
 */
class Node {
public:
    Node(std::shared_ptr<vk::Device> device, int nodeID);
    virtual ~Node() = default;

    virtual bool create() = 0;

    /*! Prepare the CommandBuffers that when executed in order will render the current frame. Updates the values
     * returned by computeCommands() and drawCommands().
     *
     * \param context The frame context for rendering this frame. Will receive command buffers and other resources that
     *        need to survive at least as long as the frame is being pipelined.
     * \return If true the primary command buffers will need to be rebuilt because of a change in one more of the cached
     *         secondary command buffers.
     */
    virtual bool prepareFrame(std::shared_ptr<FrameContext> context) = 0;

    /*! Change the parameter values for this node. If this node is a group, sets the parameter values for every node in
     * this group.
     *
     * \param namedValues A vector of pairs of parameter names and new values.
     * \param indexedValues A vector of pairs of parameter indexes and new values.
     */
    virtual void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                               const std::vector<std::pair<int, float>>& indexedValues) = 0;

    virtual void setRun(bool run) = 0;

    virtual void forEach(std::function<void(std::shared_ptr<Node> node)> f) = 0;

    struct NodeState {
        int nodeID;
        int numberOfChildren;  // -1 if a Scinth
        std::string name;  // "group" if a group, scinthDefName if a scinth
        std::vector<std::pair<std::string, float>> controlValues;
    };
    virtual void appendState(std::vector<NodeState>& nodes) = 0;

    virtual bool isGroup() const = 0;
    virtual bool isScinth() const = 0;

    int nodeID() const { return m_nodeID; }
    Group* parent() const { return m_parent; }
    void setParent(Group* parent) { m_parent = parent; }

protected:
    std::shared_ptr<vk::Device> m_device;
    int m_nodeID;
    Group* m_parent;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_NODE_HPP_
