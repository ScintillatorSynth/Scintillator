#ifndef SRC_COMP_ROOT_NODE_HPP_
#define SRC_COMP_ROOT_NODE_HPP_

#include "comp/Node.hpp"

namespace scin { namespace comp {

/*! Root object of the render tree. Maintains global objects for the render tree such as images. Creates the primary
 * command buffers and render passes.
 */
class RootNode : public Node {
public:
    RootNode();
    virtual ~RootNode();

    bool create() override;
    bool destroy() override;
    bool prepareFrame(size_t imageIndex, double frameTime) override;

protected:
};

} // namespace comp
} // namespace scin

#endif // SRC_COMP_ROOT_NODE_HPP_
