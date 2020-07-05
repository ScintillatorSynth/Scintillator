#include "base/Shape.hpp"

#include "glm/glm.hpp"

#include <vector>

namespace {
const std::vector<glm::vec2> quadVertices = { { -1.0f, 1.0f }, { -1.0f, -1.0f }, { 1.0f, 1.0f }, { 1.0f, -1.0f } };
const std::vector<glm::vec2> quadTexVertices = { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 0.0f } };
const std::vector<uint16_t> quadIndices = { 0, 1, 2, 3 };
}

namespace scin { namespace base {

Shape::Shape() {}
Shape::~Shape() {}

Quad::Quad(int widthEdges, int heightEdges): m_widthEdges(widthEdges), m_heightEdges(heightEdges) {}
Quad::~Quad() {}

Manifest::ElementType Quad::elementType() const { return Manifest::ElementType::kVec2; }

uint32_t Quad::numberOfVertices() const { return quadVertices.size(); }

uint32_t Quad::numberOfIndices() const { return quadIndices.size(); }

Shape::Topology Quad::topology() const { return kTriangleStrip; }

size_t Quad::storeVertexAtIndex(uint32_t index, float* store) const {
    store[0] = quadVertices[index].x;
    store[1] = quadVertices[index].y;
    return 2;
}

size_t Quad::storeTextureVertexAtIndex(uint32_t index, float* store) const {
    store[0] = quadTexVertices[index].x;
    store[1] = quadTexVertices[index].y;
    return 2;
}

const uint16_t* Quad::getIndices() const { return quadIndices.data(); }

} // namespace base

} // namespace scin
