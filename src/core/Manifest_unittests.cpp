#include "gtest/gtest.h"

#include "core/Manifest.hpp"

namespace scin {

TEST(ManifestTest, EmptyManifest) {
    Manifest manifest;
    EXPECT_EQ(0, manifest.numberOfElements());
    EXPECT_EQ(0, manifest.sizeInBytes());
}

} // namespace scin
