#include "base/Shape.hpp"

#include "spdlog/spdlog.h"

#include <vector>

namespace scin { namespace base {

Shape::Shape() {}
Shape::~Shape() {}

Quad::Quad(int widthEdges, int heightEdges): m_widthEdges(widthEdges), m_heightEdges(heightEdges) {}
Quad::~Quad() {}

Manifest::ElementType Quad::elementType() const { return Manifest::ElementType::kVec2; }

uint32_t Quad::numberOfVertices() const { return (m_widthEdges + 1) * (m_heightEdges + 1); }

uint32_t Quad::numberOfIndices() const {
    uint32_t indicesPerRow = (m_widthEdges + 1) * 2;
    // All but the final row adds two indices for degenerate triangles to start next row again.
    return (m_heightEdges * indicesPerRow) + ((m_heightEdges - 1) * 2);
}

Shape::Topology Quad::topology() const { return kTriangleStrip; }

// TODO: sanity check inputs aren't negative, and number of vertices doesn't exceed 64K

bool Quad::storeVertexData(const Manifest& vertexManifest, const glm::vec2& normPosScale, float* store) const {
    glm::vec2 upperLeft { -1.0f, -1.0f };

    for (auto i = 0; i <= m_heightEdges; ++i) {
        float y = static_cast<float>(i) / static_cast<float>(m_heightEdges);
        for (auto j = 0; j <= m_widthEdges; ++j) {
            glm::vec2 v { static_cast<float>(j) / static_cast<float>(m_widthEdges), y };
            for (size_t k = 0; k < vertexManifest.numberOfElements(); ++k) {
                switch (vertexManifest.intrinsicForElement(k)) {
                case kNormPos: {
                    glm::vec2 normPos = (upperLeft + (v * 2.0f)) * normPosScale;
                    store[0] = normPos.x;
                    store[1] = normPos.y;
                } break;

                case kPosition: {
                    glm::vec2 pos = upperLeft + (v * 2.0f);
                    store[0] = pos.x;
                    store[1] = pos.y;
                } break;

                case kTexPos: {
                    store[0] = v.x;
                    store[1] = v.y;
                } break;

                default:
                    spdlog::error("Unsupported vertex manifest Intrinsic in Quad");
                    return false;
                }

                store += vertexManifest.strideForElement(k) / sizeof(float);
            }
        }
    }
    return true;
}

bool Quad::storeIndexData(uint16_t* store) const {
    uint16_t widthVerts = m_widthEdges + 1;
    for (auto i = 0; i < m_heightEdges; ++i) {
        uint16_t rowStart = i * widthVerts;
        for (auto j = 0; j <= m_widthEdges; ++j) {
            uint16_t topIndex = j + rowStart;
            store[0] = topIndex;
            store[1] = topIndex + widthVerts;
            store += 2;
        }
        // For all rows but last, repeat last index of this row and first of next to start new row
        // of strips.
        if (i < m_heightEdges - 1) {
            store[0] = rowStart + widthVerts + m_widthEdges;
            store[1] = rowStart + widthVerts;
            store += 2;
        }
    }
    return true;
}

} // namespace base
} // namespace scin
