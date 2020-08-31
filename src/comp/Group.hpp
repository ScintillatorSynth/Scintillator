#ifndef SRC_COMP_GROUP_HPP_
#define SRC_COMP_GROUP_HPP_

#include "comp/Node.hpp"

#include <memory>
#include <unordered_map>
#include <vector>

namespace scin {

namespace vk {
class Device;
}

namespace comp {

class FrameContext;

/*! Used for controlling rendering order + contains a set of shared state for rendering such as if a depth buffer is
 * needed or no and a transformation matrix to apply.
 */
class Group : public Node {
public:
    Group(std::shared_ptr<vk::Device> device, int nodeID);
    virtual ~Group() = default;

    /*! Creates the Group. As Groups do no rendering on their own this is essentially a no-op.
     *
     * \return Always true to indicate success.
     */
    bool create() override;

    /*! Prepare to render the frame described in context. Will recursively call prepareFrame() on call children within
     * the group.
     *
     * \param context The state container for the requested frame to render.
     * \return true If any child of the group had to recreate their command buffers, meaning that the entire primary
     *         buffer will also need to be recreated.
     */
    bool prepareFrame(std::shared_ptr<FrameContext> context) override;

    /*! Sets parameters on all children in the group.
     *
     * \param namedValues Pairs of parameter names and values to set.
     * \param indexedValues Pairs of parameter indices and values to set.
     */
    void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                       const std::vector<std::pair<int, float>>& indexedValues) override;

    /*! Sets the run state for all children in the group.
     *
     * \param run If true will play the node, if false will pause.
     */
    void setRun(bool run) override;

    /*! Run the provided function on each child in recursive post-order traversal.
     *
     * \param f The function to run. Each node will be provided as a pointer argument to f.
     */
    void forEach(std::function<void(std::shared_ptr<Node> node)> f) override;

    /*! Add a NodeState struct to the list detailing this group's current state. Does not recurse.
     *
     * \param nodes The output vector to append the single NodeState struct to.
     */
    void appendState(std::vector<NodeState>& nodes) override;

    bool isGroup() const override { return true; }
    bool isScinth() const override { return false; }

    /*! Remove the child node with the provided ID. Does nothing recursive with any possible descendants of that node.
     *
     * \param nodeID The id of the node to remove. No bounds checking is performed on this argument.
     */
    void remove(int nodeID);

    /*! Insert the provided node directly before nodeB.
     *
     * \param a The node to insert
     * \param nodeB The ID of the node to insert a before.
     */
    void insertBefore(std::shared_ptr<Node> a, int nodeB);

    /*! Insert the provided node directly after nodeB.
     *
     * \param a The node to insert.
     * \param nodeB The ID of the node to insert a after.
     */
    void insertAfter(std::shared_ptr<Node> a, int nodeB);

    /*! Add a node to the head of this group's list.
     *
     * \param node The node to add to the head.
     */
    void prepend(std::shared_ptr<Node> node);

    /*! Add a node to the tail of this group's list.
     *
     * \param node The node to add to the tail.
     */
    void append(std::shared_ptr<Node> node);

    /*! Replace the target node with the provided one.
     *
     * \param node The new node to adopt.
     * \param target The ID of the node to replace. Note that target and node can have the same ID.
     */
    void replace(std::shared_ptr<Node> node, int target);

    /*! Free all children, recursively, leaving this Group empty.
     */
    void freeAll();

    /*! Recursively free all Scinths only, leaving contained Group structure intact.
     */
    void deepFree();

    /*! Adds a copy of the state for each child in the group (but not this group itself.)
     *
     * \param nodes The output vector to append the state structures to.
     */
    void queryTree(std::vector<Node::NodeState>& nodes);

protected:
    std::list<std::shared_ptr<Node>> m_subNodes;
    std::unordered_map<int, std::list<std::shared_ptr<Node>>::iterator> m_nodeMap;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_GROUP_HPP_
