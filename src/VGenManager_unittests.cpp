#include "gtest/gtest.h"

#include "VGenManager.hpp"
#include "core/AbstractVGen.hpp"
#include "core/FileSystem.hpp"

#include <fstream>

namespace {

void clobberFileWithString(const fs::path& filePath, const std::string& contents) {
    std::ofstream outFile(filePath, std::ofstream::out | std::ofstream::trunc);
    ASSERT_TRUE(outFile.good());
    outFile << contents;
    ASSERT_TRUE(outFile.good());
}

}

namespace scin {

TEST(VGenManagerTest, InvalidYamlStrings) {
    VGenManager manager;
    EXPECT_EQ(0, manager.parseFromString(""));
    EXPECT_EQ(0, manager.parseFromString("xxzz"));
    EXPECT_EQ(0, manager.parseFromString("[ 1 2 3 4 ]"));
    EXPECT_EQ(0, manager.parseFromString("\""));
    EXPECT_EQ(0, manager.parseFromString("\a\b\f\v\\x08\x16"));

    EXPECT_GT(0, manager.parseFromString("'f#oo: '%>^'|"));

    // Name and fragment are currently required in all VGen specs.
    EXPECT_EQ(0,
              manager.parseFromString("---\n"
                                      "name: TestBadVGen\n"
                                      "inputs:\n"
                                      "  - a\n"
                                      "  - b\n"));
    EXPECT_EQ(0,
              manager.parseFromString("---\n"
                                      "parameters:\n"
                                      "  - time\n"
                                      "fragment: \"@out = 1.0;\"\n"));
    EXPECT_EQ(0, manager.numberOfVGens());
}

TEST(VGenManagerTest, ValidYamlStrings) {
    VGenManager manager;
    EXPECT_EQ(3,
              manager.parseFromString("---\n"
                                      "name: JustNameAndFragment\n"
                                      "fragment: \"@out = 1.0;\"\n"
                                      "---\n"
                                      "name: AddInput\n"
                                      "inputs:\n"
                                      "  - a\n"
                                      "fragment: \"@out = @a;\"\n"
                                      "---\n"
                                      "name: Overwrite\n"
                                      "fragment: \"@out = 0.0;\"\n"));
    EXPECT_EQ(3, manager.numberOfVGens());
    std::shared_ptr<const AbstractVGen> justNameAndFragment = manager.getVGenNamed("JustNameAndFragment");
    ASSERT_TRUE(justNameAndFragment);
    EXPECT_EQ("JustNameAndFragment", justNameAndFragment->name());
    EXPECT_EQ("@out = 1.0;", justNameAndFragment->fragment());
    std::shared_ptr<const AbstractVGen> addInput = manager.getVGenNamed("AddInput");
    ASSERT_TRUE(addInput);
    EXPECT_EQ("AddInput", addInput->name());
    ASSERT_EQ(1, addInput->inputs().size());
    EXPECT_EQ("a", addInput->inputs()[0]);
    EXPECT_EQ("@out = @a;", addInput->fragment());
    std::shared_ptr<const AbstractVGen> overwrite = manager.getVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ("@out = 0.0;", overwrite->fragment());

    EXPECT_FALSE(manager.getVGenNamed("Nonexistent"));

    EXPECT_EQ(1,
              manager.parseFromString("---\n"
                                      "name: Overwrite\n"
                                      "parameters:\n"
                                      "  - time\n"
                                      "fragment: \"@out = @time;\"\n"));
    // Should have overwritten the first Overwrite VGen.
    EXPECT_EQ(3, manager.numberOfVGens());
    overwrite = manager.getVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    ASSERT_EQ(1, overwrite->parameters().size());
    EXPECT_EQ("time", overwrite->parameters()[0]);
    EXPECT_EQ("@out = @time;", overwrite->fragment());

    EXPECT_EQ(1,
              manager.parseFromString("---\n"
                                      "name: Overwrite\n"
                                      "intermediates:\n"
                                      "  - fl\n"
                                      "fragment: \"@fl = 2.0; @out = @fl;\"\n"));
    EXPECT_EQ(3, manager.numberOfVGens());
    overwrite = manager.getVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ(0, overwrite->parameters().size());
    ASSERT_EQ(1, overwrite->intermediates().size());
    EXPECT_EQ("fl", overwrite->intermediates()[0]);
    EXPECT_EQ("@fl = 2.0; @out = @fl;", overwrite->fragment());
}

TEST(VGenManagerTest, ParseFromFile) {
    fs::path tempFile = fs::temp_directory_path() / "VGenManager_FileTest.yaml";
    if (fs::exists(tempFile)) {
        ASSERT_TRUE(fs::remove(tempFile));
    }

    VGenManager manager;
    // Nonexistent file should return error.
    EXPECT_GT(0, manager.loadFromFile(tempFile));

    clobberFileWithString(tempFile, "'f#oo: '%>^'|");
    EXPECT_GT(0, manager.loadFromFile(tempFile));

    clobberFileWithString(tempFile,
                          "---\n"
                          "name: FileVGen\n"
                          "fragment: \"@out = 0.0f\"\n");
    EXPECT_EQ(1, manager.loadFromFile(tempFile));

    // Remove temp file.
    ASSERT_TRUE(fs::remove(tempFile));
}

} // namespace scin
