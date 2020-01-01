#include "core/Shape.hpp"

namespace scin {

Shape::Shape() {}
Shape::~Shape() {}

Quad::Quad() {}
Quad::~Quad() {}

Manifest::ElementType Quad::elementType() {
    return Manifest::ElementType::kVec2;
}

uint32_t Quad::numberOfVertices() {
    return 4;
}

Shape::Topology Quad::topology() {
    return kTriangleStrip;
}

} // namespace scin
