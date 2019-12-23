#include "gtest/gtest.h"

#include "core/AbstractScinthDef.hpp"
#include "core/AbstractVGen.hpp"
#include "core/FileSystem.hpp"
#include "core/ScinthDefParser.hpp"
#include "core/VGen.hpp"

#include <fstream>

namespace {

void populateAbstractVGens(std::shared_ptr<scin::ScinthDefParser> parser) {
    parser->parseAbstractVGensFromString("---\n"
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
}

void clobberFileWithString(const fs::path& filePath, const std::string& contents) {
    std::ofstream outFile(filePath, std::ofstream::out | std::ofstream::trunc);
    ASSERT_TRUE(outFile.good());
    outFile << contents;
    ASSERT_TRUE(outFile.good());
}

}

namespace scin {

TEST(ScinthDefParserTest, InvalidYAMLStrings) {
    std::shared_ptr<scin::ScinthDefParser> parser(new scin::ScinthDefParser());
    populateAbstractVGens(parser);
    EXPECT_EQ(0, parser->parseFromString("").size());
    EXPECT_EQ(0, parser->parseFromString("abcd").size());
    EXPECT_EQ(0, parser->parseFromString("[ a b c d ]").size());
    EXPECT_EQ(0, parser->parseFromString("'f#oo: '%>^'|").size());

    // Both name and vgens are required root keys.
    EXPECT_EQ(0, parser->parseFromString("name: NoVGens\n").size());
    EXPECT_EQ(0,
              parser
                  ->parseFromString("vgens:\n"
                                    "  - className: NoInput\n"
                                    "    rate: fragment\n")
                  .size());
    // The vgens key has to be a sequence.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("name: VGensNotASequence\n"
                                    "vgens: NopeNopeNope\n")
                  .size());

    // The vgen must exist in the dictionary.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("---\n"
                                    "name: missingVGen\n"
                                    "vgens:\n"
                                    "  - className: NotFound\n"
                                    "    rate: fragment\n")
                  .size());

    // The vgen must have the correct number of inputs.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("name: wrongInputsVGen\n"
                                    "vgens:\n"
                                    "  - className: NoInput\n"
                                    "    rate: fragment\n"
                                    "    inputs:\n"
                                    "      - type: constant\n"
                                    "        value: 200.0\n")
                  .size());

    // Bad index on vgen inputs.
    EXPECT_EQ(0,
              parser
                  ->parseFromString("name: badVGenInputsOrder\n"
                                    "vgens:\n"
                                    "  - className: OneInput\n"
                                    "    rate: fragment\n"
                                    "    inputs:\n"
                                    "      - type: vgen\n"
                                    "        vgenIndex: 1\n")
                  .size());

    EXPECT_EQ(0, parser->numberOfAbstractScinthDefs());
}

TEST(ScinthDefManagerTest, ValidYAMLStrings) {
    std::shared_ptr<scin::ScinthDefParser> parser(new scin::ScinthDefParser());
    populateAbstractVGens(parser);

    EXPECT_EQ(1,
              parser
                  ->parseFromString("---\n"
                                    "name: firstScinth\n"
                                    "vgens:\n"
                                    "  - className: NoInput\n"
                                    "    rate: fragment\n")
                  .size());
    ASSERT_EQ(1, parser->numberOfAbstractScinthDefs());
    std::shared_ptr<const AbstractScinthDef> scinthDef = parser->getAbstractScinthDefNamed("firstScinth");
    ASSERT_EQ(1, scinthDef->numberOfInstances());
    EXPECT_EQ("NoInput", scinthDef->instanceAt(0).abstractVGen()->name());
    EXPECT_EQ(0, scinthDef->instanceAt(0).numberOfInputs());

    EXPECT_EQ(2,
              parser
                  ->parseFromString("---\n"
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
                                    "        value: 3\n")
                  .size());
    ASSERT_EQ(2, parser->numberOfAbstractScinthDefs());

    scinthDef = parser->getAbstractScinthDefNamed("firstScinth");
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

    scinthDef = parser->getAbstractScinthDefNamed("secondScinth");
    ASSERT_EQ(1, scinthDef->numberOfInstances());
    EXPECT_EQ("ThreeInput", scinthDef->instanceAt(0).abstractVGen()->name());
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(0, inputValue));
    EXPECT_EQ(1.0f, inputValue);
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(1, inputValue));
    EXPECT_EQ(2.0f, inputValue);
    EXPECT_TRUE(scinthDef->instanceAt(0).getInputConstantValue(2, inputValue));
    EXPECT_EQ(3.0f, inputValue);
}

TEST(ScinthDefParserTest, InvalidAbstractVGenYamlStrings) {
    ScinthDefParser parser;
    EXPECT_EQ(0, parser.parseAbstractVGensFromString(""));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("xxzz"));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("[ 1 2 3 4 ]"));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("\""));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("\a\b\f\v\\x08\x16"));
    EXPECT_EQ(0, parser.parseAbstractVGensFromString("'f#oo: '%>^'|"));

    // Name and fragment are currently required in all VGen specs.
    EXPECT_EQ(0,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: TestBadVGen\n"
                                                  "inputs:\n"
                                                  "  - a\n"
                                                  "  - b\n"));
    EXPECT_EQ(0,
              parser.parseAbstractVGensFromString("---\n"
                                                  "parameters:\n"
                                                  "  - time\n"
                                                  "fragment: \"@out = 1.0;\"\n"));
    EXPECT_EQ(0, parser.numberOfAbstractVGens());
}

TEST(ScinthDefParserTest, ValidAbstractVGenYamlStrings) {
    ScinthDefParser parser;
    EXPECT_EQ(3,
              parser.parseAbstractVGensFromString("---\n"
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
    EXPECT_EQ(3, parser.numberOfAbstractVGens());
    std::shared_ptr<const AbstractVGen> justNameAndFragment = parser.getAbstractVGenNamed("JustNameAndFragment");
    ASSERT_TRUE(justNameAndFragment);
    EXPECT_EQ("JustNameAndFragment", justNameAndFragment->name());
    EXPECT_EQ("@out = 1.0;", justNameAndFragment->fragment());
    std::shared_ptr<const AbstractVGen> addInput = parser.getAbstractVGenNamed("AddInput");
    ASSERT_TRUE(addInput);
    EXPECT_EQ("AddInput", addInput->name());
    ASSERT_EQ(1, addInput->inputs().size());
    EXPECT_EQ("a", addInput->inputs()[0]);
    EXPECT_EQ("@out = @a;", addInput->fragment());
    std::shared_ptr<const AbstractVGen> overwrite = parser.getAbstractVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ("@out = 0.0;", overwrite->fragment());

    EXPECT_FALSE(parser.getAbstractVGenNamed("Nonexistent"));

    EXPECT_EQ(1,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: Overwrite\n"
                                                  "parameters:\n"
                                                  "  - time\n"
                                                  "fragment: \"@out = @time;\"\n"));
    // Should have overwritten the first Overwrite VGen.
    EXPECT_EQ(3, parser.numberOfAbstractVGens());
    overwrite = parser.getAbstractVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    ASSERT_EQ(1, overwrite->parameters().size());
    EXPECT_EQ("time", overwrite->parameters()[0]);
    EXPECT_EQ("@out = @time;", overwrite->fragment());

    EXPECT_EQ(1,
              parser.parseAbstractVGensFromString("---\n"
                                                  "name: Overwrite\n"
                                                  "intermediates:\n"
                                                  "  - fl\n"
                                                  "fragment: \"@fl = 2.0; @out = @fl;\"\n"));
    EXPECT_EQ(3, parser.numberOfAbstractVGens());
    overwrite = parser.getAbstractVGenNamed("Overwrite");
    ASSERT_TRUE(overwrite);
    EXPECT_EQ("Overwrite", overwrite->name());
    EXPECT_EQ(0, overwrite->parameters().size());
    ASSERT_EQ(1, overwrite->intermediates().size());
    EXPECT_EQ("fl", overwrite->intermediates()[0]);
    EXPECT_EQ("@fl = 2.0; @out = @fl;", overwrite->fragment());
}

TEST(ScinthDefParserTest, ParseAbstractVGenFromFile) {
    fs::path tempFile = fs::temp_directory_path() / "ScinthDefParser_FileTest.yaml";
    if (fs::exists(tempFile)) {
        ASSERT_TRUE(fs::remove(tempFile));
    }

    ScinthDefParser parser;
    // Nonexistent file should not parse.
    EXPECT_EQ(0, parser.loadAbstractVGensFromFile(tempFile));

    clobberFileWithString(tempFile, "'f#oo: '%>^'|");
    EXPECT_EQ(0, parser.loadAbstractVGensFromFile(tempFile));

    clobberFileWithString(tempFile,
                          "---\n"
                          "name: FileVGen\n"
                          "fragment: \"@out = 0.0f\"\n");
    EXPECT_EQ(1, parser.loadAbstractVGensFromFile(tempFile));

    // Remove temp file.
    ASSERT_TRUE(fs::remove(tempFile));
}

} // namespace scin
