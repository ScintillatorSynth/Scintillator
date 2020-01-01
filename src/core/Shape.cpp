#include "core/Shape.hpp"

namespace scin {

Shape::Shape() {}
Shape::~Shape() {}

Quad::Quad() {}
Quad::~Quad() {}

Manifest::ElementType Quad::elementType() const { return Manifest::ElementType::kVec2; }

uint32_t Quad::numberOfVertices() const { return 4; }

Shape::Topology Quad::topology() const { return kTriangleStrip; }

} // namespace scin
