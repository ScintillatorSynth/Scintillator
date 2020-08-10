#ifndef SRC_COMP_GROUP_HPP_
#define SRC_COMP_GROUP_HPP_

#include "comp/Node.hpp"

#include <memory>
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
     * \namedValues Pairs of parameter names and values to set.
     * \indexedValues Pairs of parameter indices and values to set.
     */
    void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                       const std::vector<std::pair<int, float>>& indexedValues) override;

    void setRun(bool run) override;

    bool isGroup() const override { return true; }
    bool isScinth() const override { return false; }

    void subNodeFree(int nodeID);
    void insertBefore(std::shared_ptr<Node> a, int nodeB);
    void insertAfter(std::shared_ptr<Node> a, int nodeB);
    void prepend(std::shared_ptr<Node> node);
    void append(std::shared_ptr<Node> node);


protected:
    std::list<std::shared_ptr<Node>> m_subNodes;
    std::unordered_map<int, std::list<std::shared_ptr<Node>::iterator> m_nodeMap;
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_GROUP_HPP_
