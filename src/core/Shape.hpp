#ifndef SRC_CORE_SHAPE_HPP_
#define SRC_CORE_SHAPE_HPP_

#include "core/Manifest.hpp"

namespace scin {

/*! Abstract base class representing a geometric shape used as the starting point for rendering.
 */
class Shape {
public:
    Shape();
    virtual ~Shape();

    enum Topology { kTriangleStrip };

    virtual Manifest::ElementType elementType() = 0;
    virtual uint32_t numberOfVertices() = 0;
    virtual Topology topology() = 0;
};

class Quad : public Shape {
public:
    Quad();
    virtual ~Quad();

    Manifest::ElementType elementType() override;
    uint32_t numberOfVertices() override;
    Shape::Topology topology() override;
};

}

#endif // SRC_CORE_SHAPE_HPP_
