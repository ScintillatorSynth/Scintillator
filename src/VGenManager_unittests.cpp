#include "gtest/gtest.h"

#include "FileSystem.hpp"
#include "VGen.hpp"
#include "VGenManager.hpp"


namespace scin {

TEST(VGenManagerTest, InvalidYamlStrings) {
    VGenManager manager;
    EXPECT_EQ(0, manager.parseFromString(""));
    EXPECT_EQ(0, manager.parseFromString("xxzz"));
    EXPECT_EQ(0, manager.parseFromString("[ 1 2 3 4 ]"));
    EXPECT_EQ(0, manager.parseFromString("\""));
    // Name and fragment are currently required in all VGen specs.
    EXPECT_EQ(0, manager.parseFromString(
        "---\n"
        "name: TestBadVGen\n"
        "inputs:\n"
        "  - a\n"
        "  - b\n"
    ));
    EXPECT_EQ(0, manager.parseFromString(
        "---\n"
        "parameters:\n"
        "  - time\n"
        "fragment: \"@out = 1.0;\"\n"
    ));
    EXPECT_EQ(0, manager.numberOfVGens());
}

TEST(VGenManagerTest, ValidYamlStrings) {
    VGenManager manager;
    EXPECT_EQ(3, manager.parseFromString(
        "---\n"
        "name: JustNameAndFragment\n"
        "fragment: \"@out = 1.0;\"\n"
        "---\n"
        "name: AddInput\n"
        "inputs:\n"
        "  - a\n"
        "fragment: \"@out = @a;\"\n"
        "---\n"
        "name: Overwrite\n"
        "fragment: \"@out = 0.0;\"\n"
    ));
    EXPECT_EQ(3, manager.numberOfVGens());
    std::shared_ptr<VGen> justNameAndFragment = manager.getVGenNamed("JustNameAndFragment");
    ASSERT_TRUE(justNameAndFragment);
    EXPECT_EQ("JustNameAndFragment", justNameAndFragment->name());
    EXPECT_EQ("@out = 1.0;", justNameAndFragment->fragment());
    std::shared_ptr<VGen> addInput = manager.getVGenNamed("AddInput");
    ASSERT_TRUE(addInput);
    EXPECT_EQ("AddInput", addInput->name());
    ASSERT_EQ(1, addInput->inputs().size());
    EXPECT_EQ("a", addInput->inputs()[0]);
    EXPECT_EQ("@out = @a;", addInput->fragment());
    std::shared_ptr<VGen> overwrite = manager.getVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ("@out = 0.0;", overwrite->fragment());

    EXPECT_FALSE(manager.getVGenNamed("Nonexistent"));

    EXPECT_EQ(1, manager.parseFromString(
        "---\n"
        "name: Overwrite\n"
        "parameters:\n"
        "  - time\n"
        "fragment: \"@out = @time;\"\n"
    ));
    // Should have overwritten the first Overwrite VGen.
    EXPECT_EQ(3, manager.numberOfVGens());
    overwrite = manager.getVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    ASSERT_EQ(1, overwrite->parameters().size());
    EXPECT_EQ("time", overwrite->parameters()[0]);
    EXPECT_EQ("@out = @time;", overwrite->fragment());

    EXPECT_EQ(1, manager.parseFromString(
        "---\n"
        "name: Overwrite\n"
        "intermediates:\n"
        "  - fl\n"
        "fragment: \"@fl = 2.0; @out = @fl;\"\n"
    ));
    EXPECT_EQ(3, manager.numberOfVGens());
    overwrite = manager.getVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ(0, overwrite->parameters().size());
    ASSERT_EQ(1, overwrite->intermediates().size());
    EXPECT_EQ("fl", overwrite->intermediates()[0]);
    EXPECT_EQ("@fl = 2.0; @out = @fl;", overwrite->fragment());
}

// TODO:
//  (a) add test coverage metrics - consider LLVM for general compilation?
//  (b) add file test using temporary file, to show metrics going up
//  (c) add any other tests to get VGenManager test coverage up to 100%
//  (d) Build ScinSynth parser/generator/compiler

}  // namespace scin

