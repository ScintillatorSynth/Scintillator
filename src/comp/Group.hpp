#ifndef SRC_COMP_GROUP_HPP_
#define SRC_COMP_GROUP_HPP_

#include "comp/Node.hpp"

#include <list>
#include <memory>

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
    virtual ~Group;

    bool create() override;
    void destroy() override;
    bool prepareFrame(std::shared_ptr<FrameContext> context) override;
    void setParameters(const std::vector<std::pair<std::string, float>>& namedValues,
                       const std::vector<std::pair<int, float>> * indexedValues) override;

protected:
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_GROUP_HPP_

