#include "gtest/gtest.h"

#include "ScinthDef.hpp"
#include "ScinthDefManager.hpp"
#include "VGenManager.hpp"
#include "core/AbstractVGen.hpp"
#include "core/FileSystem.hpp"
#include "core/VGen.hpp"

#include <fstream>
#include <memory>

namespace {

std::shared_ptr<scin::VGenManager> makeVGenManager() {
    std::shared_ptr<scin::VGenManager> vgenManager(new scin::VGenManager());
    vgenManager->parseFromString("---\n"
                                 "name: NoInput\n"
                                 "fragment: \"@out = 1.0f;\"\n"
                                 "---\n"
                                 "name: OneInput\n"
                                 "inputs:\n"
                                 "  - a\n"
                                 "fragment: \"@out = @a;\"\n"
                                 "---\n"
                                 "name: TwoInput\n"
                                 "inputs:\n"
                                 "  - a\n"
                                 "  - b\n"
                                 "fragment: \"@out = @a * @b;\"\n"
                                 "---\n"
                                 "name: ThreeInput\n"
                                 "inputs:\n"
                                 "  - a\n"
                                 "  - b\n"
                                 "  - c\n"
                                 "fragment: \"@out = @a + @b + @c;\"\n");
    if (vgenManager->numberOfVGens() != 4)
        return nullptr;
    return vgenManager;
}

void clobberFileWithString(const fs::path& filePath, const std::string& contents) {
    std::ofstream outFile(filePath, std::ofstream::out | std::ofstream::trunc);
    ASSERT_TRUE(outFile.good());
    outFile << contents;
    ASSERT_TRUE(outFile.good());
}

} // namespace

namespace scin {

TEST(ScinthDefManagerTest, InvalidYAMLStrings) {
    std::shared_ptr<scin::VGenManager> vgenManager = makeVGenManager();
    ScinthDefManager manager(vgenManager);
    EXPECT_EQ(0, manager.parseFromString(""));
    EXPECT_EQ(0, manager.parseFromString("abcd"));
    EXPECT_EQ(0, manager.parseFromString("[ a b c d ]"));

    EXPECT_GT(0, manager.parseFromString("'f#oo: '%>^'|"));

    // Both name and vgens are required root keys.
    EXPECT_EQ(0, manager.parseFromString("name: NoVGens\n"));
    EXPECT_EQ(0,
              manager.parseFromString("vgens:\n"
                                      "  - className: NoInput\n"
                                      "    rate: fragment\n"));
    // The vgens key has to be a sequence.
    EXPECT_EQ(0,
              manager.parseFromString("name: VGensNotASequence\n"
                                      "vgens: NopeNopeNope\n"));

    // The vgen must exist in the dictionary.
    EXPECT_EQ(0,
              manager.parseFromString("---\n"
                                      "name: missingVGen\n"
                                      "vgens:\n"
                                      "  - className: NotFound\n"
                                      "    rate: fragment\n"));

    // The vgen must have the correct number of inputs.
    EXPECT_EQ(0,
              manager.parseFromString("name: wrongInputsVGen\n"
                                      "vgens:\n"
                                      "  - className: NoInput\n"
                                      "    rate: fragment\n"
                                      "    inputs:\n"
                                      "      - type: constant\n"
                                      "        value: 200.0\n"));

    // Bad index on vgen inputs.
    EXPECT_EQ(0,
              manager.parseFromString("name: badVGenInputsOrder\n"
                                      "vgens:\n"
                                      "  - className: OneInput\n"
                                      "    rate: fragment\n"
                                      "    inputs:\n"
                                      "      - type: vgen\n"
                                      "        vgenIndex: 1\n"));

    EXPECT_EQ(0, manager.numberOfScinthDefs());
}

TEST(ScinthDefManagerTest, ValidYAMLStrings) {
    std::shared_ptr<scin::VGenManager> vgenManager = makeVGenManager();
    ScinthDefManager manager(vgenManager);

    EXPECT_EQ(1,
              manager.parseFromString("---\n"
                                      "name: firstScinth\n"
                                      "vgens:\n"
                                      "  - className: NoInput\n"
                                      "    rate: fragment\n"));
    ASSERT_EQ(1, manager.numberOfScinthDefs());
    std::shared_ptr<const ScinthDef> scinthDef = manager.getScinthDefNamed("firstScinth");
    ASSERT_EQ(1, scinthDef->numberOfInstances());
    EXPECT_EQ("NoInput", scinthDef->instanceAt(0).abstractVGen()->name());
    EXPECT_EQ(0, scinthDef->instanceAt(0).numberOfInputs());

    EXPECT_EQ(2,
              manager.parseFromString("---\n"
                                      "name: firstScinth\n"
                                      "vgens:\n"
                                      "  - className: OneInput\n"
                                      "    rate: fragment\n"
                                      "    inputs:\n"
                                      "      - type: constant\n"
                                      "        value: -123.0\n"
                                      "  - className: TwoInput\n"
                                      "    rate: fragment\n"
                                      "    inputs:\n"
                                      "      - type: vgen\n"
                                      "        vgenIndex: 0\n"
                                      "      - type: constant\n"
                                      "        value: 0.0\n"
                                      "---\n"
                                      "name: secondScinth\n"
                                      "vgens: \n"
                                      "  - className: ThreeInput\n"
                                      "    rate: fragment\n"
                                      "    inputs: \n"
                                      "      - type: constant\n"
                                      "        value: 1\n"
                                      "      - type: constant\n"
                                      "        value: 2\n"
                                      "      - type: constant\n"
                                      "        value: 3\n"));
    ASSERT_EQ(2, manager.numberOfScinthDefs());

    scinthDef = manager.getScinthDefNamed("firstScinth");
    ASSERT_EQ(2, scinthDef->numberOfInstances());
    EXPECT_EQ("OneInput", scinthDef->instanceAt(0).abstractVGen()->name());
    ASSERT_EQ(1, scinthDef->instanceAt(0).numberOfInputs());
    float inputValue = 0.0f;
    int vgenIndex = -1;
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(0, inputValue));
    EXPECT_FALSE(scinthDef->instanceAt(0).getInputVGenIndex(0, vgenIndex));
    EXPECT_EQ(-123.0f, inputValue);
    EXPECT_EQ(-1, vgenIndex); // vgenIndex should remain unmodified as the input is a constant.
    EXPECT_EQ("TwoInput", scinthDef->instanceAt(1).abstractVGen()->name());
    ASSERT_EQ(2, scinthDef->instanceAt(1).numberOfInputs());
    EXPECT_FALSE(scinthDef->instanceAt(1).getInputConstantValue(0, inputValue));
    EXPECT_TRUE(scinthDef->instanceAt(1).getInputVGenIndex(0, vgenIndex));
    EXPECT_EQ(-123.0f, inputValue); // inputValue should remain unmodified as the input is a VGen.
    EXPECT_EQ(0, vgenIndex);

    scinthDef = manager.getScinthDefNamed("secondScinth");
    ASSERT_EQ(1, scinthDef->numberOfInstances());
    EXPECT_EQ("ThreeInput", scinthDef->instanceAt(0).abstractVGen()->name());
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(0, inputValue));
    EXPECT_EQ(1.0f, inputValue);
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(1, inputValue));
    EXPECT_EQ(2.0f, inputValue);
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(2, inputValue));
    EXPECT_EQ(3.0f, inputValue);
}

} // namespace scin
