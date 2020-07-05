#include "gtest/gtest.h"

#include "base/Shape.hpp"

#include "glm/glm.hpp"

namespace scin { namespace base {

TEST(QuadTest, SingleEdgesInWidthAndHeight) {
    std::unique_ptr<Quad> quad(new Quad(1, 1));

    // Expecting 4 vertices describing the extrema of clip space.
    ASSERT_EQ(4, quad->numberOfVertices());
    ASSERT_EQ(4, quad->numberOfIndices());

    std::array<float, 2> verts;
    // Vertices should run from bottom left.
    quad->storeVertexAtIndex(0, verts.data());
    EXPECT_EQ(-1.0f, verts[0]);
    EXPECT_EQ(1.0f, verts[1]);
    // To top left.
    quad->storeVertexAtIndex(1, verts.data());
    EXPECT_EQ(-1.0f, verts[0]);
    EXPECT_EQ(-1.0f, verts[1]);
    // To bottom right.
    quad->storeVertexAtIndex(2, verts.data());
    EXPECT_EQ(1.0f, verts[0]);
    EXPECT_EQ(1.0f, verts[1]);
    // To top right.
    quad->storeVertexAtIndex(3, verts.data());
    EXPECT_EQ(1.0f, verts[0]);
    EXPECT_EQ(-1.0f, verts[1]);

    // Indices should run from 0-3
    const uint16_t* indices = quad->getIndices();
    for (auto i = 0; i < 4; ++i) {
        EXPECT_EQ(i, indices[i]);
    }
}

} // namespace base
} // namespace scin
