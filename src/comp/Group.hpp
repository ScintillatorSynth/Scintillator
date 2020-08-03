#ifndef SRC_COMP_GROUP_HPP_
#define SRC_COMP_GROUP_HPP_

#include "comp/Node.hpp"

/*! Used for controlling rendering order + contains a set of shared state for rendering such as if a depth buffer is
 * needed or no and a transformation matrix to apply.
 */
class Group : public Node {
public:
    Group(int nodeID);
    virtual ~Group;

    bool create() override;
    bool destroy() override;
    bool prepareFrame(size_t imageIndex, double frameTime) override;

protected:
};

#endif // SRC_COMP_GROUP_HPP_

