#include "gtest/gtest.h"

#include "base/Manifest.hpp"
#include "base/Shape.hpp"

#include "glm/glm.hpp"

#include <array>
#include <memory>

namespace scin { namespace base {

TEST(QuadTest, SingleEdgesInWidthAndHeight) {
    std::unique_ptr<Quad> quad(new Quad(1, 1));

    // Expecting 4 vertices describing the extrema of clip space.
    ASSERT_EQ(4, quad->numberOfVertices());
    ASSERT_EQ(4, quad->numberOfIndices());

    Manifest manifest;
    manifest.addElement("position", Manifest::ElementType::kVec2, Intrinsic::kPosition);
    manifest.addElement("normPos", Manifest::ElementType::kVec2, Intrinsic::kNormPos);
    manifest.addElement("texPos", Manifest::ElementType::kVec2, Intrinsic::kTexPos);
    manifest.pack();

    std::unique_ptr<float[]> vertexData(new float[4 * manifest.sizeInBytes() / sizeof(float)]);
    ASSERT_TRUE(quad->storeVertexData(manifest, glm::vec2 { 2.0f, 0.5f }, vertexData.get()));

    // Vertices should run from left to right, top to bottom, like reading a page of English text. So first vertex
    // should describe top left.
    const float* verts = vertexData.get();
    for (auto i = 0; i < manifest.numberOfElements(); ++i) {
        switch (manifest.intrinsicForElement(i)) {
        case Intrinsic::kPosition:
            EXPECT_EQ(-1.0f, verts[0]);
            EXPECT_EQ(-1.0f, verts[1]);
            break;

        case Intrinsic::kNormPos:
            EXPECT_EQ(-2.0f, verts[0]);
            EXPECT_EQ(-0.5f, verts[1]);
            break;

        case Intrinsic::kTexPos:
            EXPECT_EQ(0.0f, verts[0]);
            EXPECT_EQ(0.0f, verts[1]);
            break;
        }

        verts += manifest.strideForElement(i) / sizeof(float);
    }

    // Second vertex should describe top right vertex.
    verts = vertexData.get() + (manifest.sizeInBytes() / sizeof(float));
    for (auto i = 0; i < manifest.numberOfElements(); ++i) {
        switch (manifest.intrinsicForElement(i)) {
        case Intrinsic::kPosition:
            EXPECT_EQ(1.0f, verts[0]);
            EXPECT_EQ(-1.0f, verts[1]);
            break;

        case Intrinsic::kNormPos:
            EXPECT_EQ(2.0f, verts[0]);
            EXPECT_EQ(-0.5f, verts[1]);
            break;

        case Intrinsic::kTexPos:
            EXPECT_EQ(1.0f, verts[0]);
            EXPECT_EQ(0.0f, verts[1]);
            break;
        }

        verts += manifest.strideForElement(i) / sizeof(float);
    }

    // Third vertex should describe lower left vertex.
    verts = vertexData.get() + (2 * manifest.sizeInBytes() / sizeof(float));
    for (auto i = 0; i < manifest.numberOfElements(); ++i) {
        switch (manifest.intrinsicForElement(i)) {
        case Intrinsic::kPosition:
            EXPECT_EQ(-1.0f, verts[0]);
            EXPECT_EQ(1.0f, verts[1]);
            break;

        case Intrinsic::kNormPos:
            EXPECT_EQ(-2.0f, verts[0]);
            EXPECT_EQ(0.5f, verts[1]);
            break;

        case Intrinsic::kTexPos:
            EXPECT_EQ(0.0f, verts[0]);
            EXPECT_EQ(1.0f, verts[1]);
            break;
        }

        verts += manifest.strideForElement(i) / sizeof(float);
    }

    // Fourth vertex should describe lower right vertex.
    verts = vertexData.get() + (3 * manifest.sizeInBytes() / sizeof(float));
    for (auto i = 0; i < manifest.numberOfElements(); ++i) {
        switch (manifest.intrinsicForElement(i)) {
        case Intrinsic::kPosition:
            EXPECT_EQ(1.0f, verts[0]);
            EXPECT_EQ(1.0f, verts[1]);
            break;

        case Intrinsic::kNormPos:
            EXPECT_EQ(2.0f, verts[0]);
            EXPECT_EQ(0.5f, verts[1]);
            break;

        case Intrinsic::kTexPos:
            EXPECT_EQ(1.0f, verts[0]);
            EXPECT_EQ(1.0f, verts[1]);
            break;
        }

        verts += manifest.strideForElement(i) / sizeof(float);
    }

    // Indices should just run 0, 2, 1, 3
    std::array<uint16_t, 4> indexData;
    quad->storeIndexData(indexData.data());
    EXPECT_EQ(0, indexData[0]);
    EXPECT_EQ(2, indexData[1]);
    EXPECT_EQ(1, indexData[2]);
    EXPECT_EQ(3, indexData[3]);
}

} // namespace base
} // namespace scin
