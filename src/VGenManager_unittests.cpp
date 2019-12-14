#include "gtest/gtest.h"

#include "VGenManager.hpp"

#include <filesystem>

namespace scin {

TEST(VGenManagerTest, InvalidYamlStrings) {
    VGenManager manager;
    EXPECT_TRUE(manager.parseFromString(""));
    EXPECT_FALSE(manager.parseFromString("xxzz"));
    EXPECT_FALSE(manager.parseFromString("[ 1 2 3 4 ]"));
}


}  // namespace scin

